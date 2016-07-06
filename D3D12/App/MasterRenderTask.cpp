#include "MasterRenderTask.h"

#include <DirectXColors.h>
#include <tbb/parallel_for.h>

#include <Camera/Camera.h>
#include <CommandManager/CommandManager.h>
#include <DXUtils/d3dx12.h>
#include <GlobalData/D3dData.h>
#include <PSOManager\PSOManager.h>
#include <ResourceManager\ResourceManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <ShaderManager\ShaderManager.h>
#include <Utils/DebugUtils.h>

namespace {
	const std::uint32_t MAX_NUM_CMD_LISTS{ 3U };
}

tbb::empty_task* MasterRenderTask::Create(MasterRenderTask* &masterRenderTask) {
	tbb::empty_task* parent{ new (tbb::task::allocate_root()) tbb::empty_task };
	parent->set_ref_count(2);
	masterRenderTask = new (parent->allocate_child()) MasterRenderTask();
	return parent;
}

void MasterRenderTask::Init(const HWND hwnd) noexcept {
	mHwnd = hwnd;

	mTimer.Reset();

	D3dData::InitDirect3D();
	InitSystems();

	ResourceManager::gManager->CreateFence(0U, D3D12_FENCE_FLAG_NONE, mFence);
	CreateCommandObjects();
	D3dData::CreateSwapChain(mHwnd, *mCmdQueue);
	CreateRtvAndDsvDescriptorHeaps();
	CreateRtvAndDsv();

	// Create and spawn command list processor thread.
	mCmdListProcessorParent = CommandListProcessor::Create(mCmdListProcessor, mCmdQueue, MAX_NUM_CMD_LISTS);
	mCmdListProcessorParent->spawn(*mCmdListProcessor);
}

void MasterRenderTask::InitSystems() noexcept {
	ASSERT(CommandManager::gManager.get() == nullptr);
	CommandManager::gManager = std::make_unique<CommandManager>(*D3dData::mDevice.Get());

	ASSERT(PSOManager::gManager.get() == nullptr);
	PSOManager::gManager = std::make_unique<PSOManager>(*D3dData::mDevice.Get());

	ASSERT(ResourceManager::gManager.get() == nullptr);
	ResourceManager::gManager = std::make_unique<ResourceManager>(*D3dData::mDevice.Get());

	ASSERT(RootSignatureManager::gManager.get() == nullptr);
	RootSignatureManager::gManager = std::make_unique<RootSignatureManager>(*D3dData::mDevice.Get());

	ASSERT(ShaderManager::gManager.get() == nullptr);
	ShaderManager::gManager = std::make_unique<ShaderManager>();
}

void MasterRenderTask::ExecuteInitTasks() noexcept {
	ASSERT(!mInitTasks.empty());
	ASSERT(mInitTasks.size() == mCmdBuilderTasks.size());

	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue{ mCmdListProcessor->CmdListQueue() };
	ASSERT(mCmdListProcessor->IsIdle());

	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, mInitTasks.size()),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
			mInitTasks[i]->Execute(*D3dData::mDevice.Get(), cmdListQueue, mCmdBuilderTasks[i]->TaskInput());
	}
	);

	while (!mCmdListProcessor->IsIdle()) {
		Sleep(0U);
	}

	FlushCommandQueue();

	const std::uint64_t count{ _countof(mFenceByFrameIndex) };
	for (std::uint64_t i = 0UL; i < count; ++i) {
		mFenceByFrameIndex[i] = mCurrentFence;
	}
}

tbb::task* MasterRenderTask::execute() {
	ExecuteCmdBuilderTasks();
	Finalize();		
	return nullptr;
}

void MasterRenderTask::ExecuteCmdBuilderTasks() noexcept {
	while (!mTerminate) {
		mTimer.Tick();
		CalculateFrameStats();
		Camera::gCamera->UpdateViewMatrix();

		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue{ mCmdListProcessor->CmdListQueue() };
		ASSERT(mCmdListProcessor->IsIdle());

		// Begin Frame task + # cmd build tasks
		const std::uint32_t taskCount{ (std::uint32_t)mCmdBuilderTasks.size() + 1U };
		mCmdListProcessor->resetExecutedTasksCounter();

		const std::uint32_t currBackBuffer{ D3dData::CurrentBackBufferIndex() };
		ID3D12CommandAllocator* cmdAllocFrameBegin{ mCmdAllocFrameBegin[currBackBuffer] };
		ID3D12CommandAllocator* cmdAllocFrameEnd{ mCmdAllocFrameEnd[currBackBuffer] };

		// Reuse the memory associated with command recording.
		// We can only reset when the associated command lists have finished execution on the GPU.
		CHECK_HR(cmdAllocFrameBegin->Reset());

		// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
		// Reusing the command list reuses memory.
		CHECK_HR(mCmdListFrameBegin->Reset(cmdAllocFrameBegin, nullptr));

		// Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
		mCmdListFrameBegin->RSSetViewports(1U, &Settings::sScreenViewport);
		mCmdListFrameBegin->RSSetScissorRects(1U, &Settings::sScissorRect);

		// Indicate a state transition on the resource usage.
		CD3DX12_RESOURCE_BARRIER resBarrier{ CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET) };
		mCmdListFrameBegin->ResourceBarrier(1, &resBarrier);

		// Specify the buffers we are going to render to.
		const D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle = CurrentBackBufferView();
		const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = DepthStencilView();
		mCmdListFrameBegin->OMSetRenderTargets(1U, &backBufferHandle, true, &dsvHandle);

		// Clear the back buffer and depth buffer.
		mCmdListFrameBegin->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0U, nullptr);
		mCmdListFrameBegin->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0U, 0U, nullptr);

		// Done recording commands.
		CHECK_HR(mCmdListFrameBegin->Close());
		cmdListQueue.push(mCmdListFrameBegin);

		tbb::parallel_for(tbb::blocked_range<std::size_t>(0, mCmdBuilderTasks.size()),
			[&](const tbb::blocked_range<size_t>& r) {
			for (size_t i = r.begin(); i != r.end(); ++i)
				mCmdBuilderTasks[i]->Execute(cmdListQueue, currBackBuffer, backBufferHandle, dsvHandle);
		}
		);

		CHECK_HR(cmdAllocFrameEnd->Reset());

		// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
		// Reusing the command list reuses memory.
		CHECK_HR(mCmdListFrameEnd->Reset(cmdAllocFrameEnd, nullptr));

		// Indicate a state transition on the resource usage.
		resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		mCmdListFrameEnd->ResourceBarrier(1U, &resBarrier);

		// Done recording commands.
		CHECK_HR(mCmdListFrameEnd->Close());

		// Wait until all previous tasks command lists are executed, before
		// executing frame end command list
		while (mCmdListProcessor->ExecutedTasksCounter() < taskCount) {
			Sleep(0U);
		}

		{
			ID3D12CommandList* cmdLists[] = { mCmdListFrameEnd };
			mCmdQueue->ExecuteCommandLists(1, cmdLists);
		}

		SignalFenceAndPresent();
	}
}

void MasterRenderTask::Finalize() noexcept {
	mCmdListProcessor->Terminate();
	mCmdListProcessorParent->wait_for_all();
	FlushCommandQueue();
}

void MasterRenderTask::CreateRtvAndDsv() noexcept {
	ASSERT(D3dData::mDevice != nullptr);
	ASSERT(D3dData::mSwapChain != nullptr);

	// Setup RTV descriptor to specify sRGB format.
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = Settings::sRTVFormats[0U];
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	const std::uint32_t rtvDescSize{ D3dData::mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) };

	for (std::uint32_t i = 0U; i < Settings::sSwapChainBufferCount; ++i) {
		CHECK_HR(D3dData::mSwapChain->GetBuffer(i, IID_PPV_ARGS(mSwapChainBuffer[i].GetAddressOf())));
		D3dData::mDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), &rtvDesc, rtvHeapHandle);
		rtvHeapHandle.Offset(1U, rtvDescSize);
	}

	// Create the depth/stencil buffer and view.
	D3D12_RESOURCE_DESC depthStencilDesc = {};
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0U;
	depthStencilDesc.Width = Settings::sWindowWidth;
	depthStencilDesc.Height = Settings::sWindowHeight;
	depthStencilDesc.DepthOrArraySize = 1U;
	depthStencilDesc.MipLevels = 1U;
	depthStencilDesc.Format = Settings::sDepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = 1U;
	depthStencilDesc.SampleDesc.Quality = 0U;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear = {};
	optClear.Format = Settings::sDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0U;
	CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
	ResourceManager::gManager->CreateCommittedResource(heapProps, D3D12_HEAP_FLAG_NONE, depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, optClear, mDepthStencilBuffer);

	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	D3dData::mDevice->CreateDepthStencilView(mDepthStencilBuffer, nullptr, DepthStencilView());
}

void MasterRenderTask::CreateCommandObjects() noexcept {
	ASSERT(Settings::sSwapChainBufferCount > 0U);

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CommandManager::gManager->CreateCmdQueue(queueDesc, mCmdQueue);

	for (std::uint32_t i = 0U; i < Settings::sSwapChainBufferCount; ++i) {
		CommandManager::gManager->CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAllocFrameBegin[i]);
		CommandManager::gManager->CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAllocFrameEnd[i]);
	}
	CommandManager::gManager->CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *mCmdAllocFrameBegin[0], mCmdListFrameBegin);
	CommandManager::gManager->CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *mCmdAllocFrameEnd[0], mCmdListFrameEnd);

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	mCmdListFrameBegin->Close();
	mCmdListFrameEnd->Close();
}

void MasterRenderTask::CalculateFrameStats() noexcept {
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.

	static std::uint32_t frameCnt{ 0U };
	static float timeElapsed{ 0.0f };

	++frameCnt;

	// Compute averages over one second period.
	if ((mTimer.TotalTime() - timeElapsed) > 1.0f) {
		const float mspf{ 1000.0f / frameCnt };
		SetWindowText(mHwnd, std::to_wstring(mspf).c_str());

		// Reset for next average.
		frameCnt = 0U;
		timeElapsed += 1.0f;
	}
}

void MasterRenderTask::CreateRtvAndDsvDescriptorHeaps() noexcept {
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = Settings::sSwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ResourceManager::gManager->CreateDescriptorHeap(rtvHeapDesc, mRtvHeap);

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1U;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0U;
	ResourceManager::gManager->CreateDescriptorHeap(dsvHeapDesc, mDsvHeap);
}

ID3D12Resource* MasterRenderTask::CurrentBackBuffer() const noexcept {
	const std::uint32_t currBackBuffer{ D3dData::mSwapChain->GetCurrentBackBufferIndex() };
	return mSwapChainBuffer[currBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE MasterRenderTask::CurrentBackBufferView() const noexcept {
	const std::uint32_t rtvDescSize{ D3dData::mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) };
	const std::uint32_t currBackBuffer{ D3dData::CurrentBackBufferIndex() };
	return D3D12_CPU_DESCRIPTOR_HANDLE{ mRtvHeap->GetCPUDescriptorHandleForHeapStart().ptr + currBackBuffer * rtvDescSize };
}

D3D12_CPU_DESCRIPTOR_HANDLE MasterRenderTask::DepthStencilView() const noexcept {
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void MasterRenderTask::FlushCommandQueue() noexcept {
	++mCurrentFence;

	CHECK_HR(mCmdQueue->Signal(mFence, mCurrentFence));

	// Wait until the GPU has completed commands up to this fence point.
	if (mFence->GetCompletedValue() < mCurrentFence) {
		const HANDLE eventHandle{ CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS) };
		ASSERT(eventHandle);

		// Fire event when GPU hits current fence.  
		CHECK_HR(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void MasterRenderTask::SignalFenceAndPresent() noexcept {
	ASSERT(D3dData::mSwapChain.Get());
	CHECK_HR(D3dData::mSwapChain->Present(0U, 0U));

	// Advance the fence value to mark commands up to this fence point.
	const std::uint32_t currBackBuffer{ D3dData::CurrentBackBufferIndex() };
	mFenceByFrameIndex[currBackBuffer] = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	CHECK_HR(mCmdQueue->Signal(mFence, mCurrentFence));

	// If we executed command lists for all frames, then we need to wait
	// at least 1 of them to be completed, before continue generating command list for a frame. 
	const std::uint64_t fence{ mFenceByFrameIndex[currBackBuffer] };
	const std::uint64_t completedFenceValue{ mFence->GetCompletedValue() };
	if (completedFenceValue < fence) {
		const HANDLE eventHandle{ CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS) };
		ASSERT(eventHandle);

		// Fire event when GPU hits current fence.  
		CHECK_HR(mFence->SetEventOnCompletion(fence, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		const std::uint64_t newCompletedFenceValue{ mFence->GetCompletedValue() };
		CloseHandle(eventHandle);
	}
}

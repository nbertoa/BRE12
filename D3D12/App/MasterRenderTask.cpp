#include "MasterRenderTask.h"

#include <DirectXColors.h>
#include <tbb/parallel_for.h>

#include <App/Scene.h>
#include <Camera/Camera.h>
#include <CommandManager/CommandManager.h>
#include <DXUtils/d3dx12.h>
#include <GlobalData/D3dData.h>
#include <Input/Keyboard.h>
#include <Input/Mouse.h>
#include <PSOManager\PSOManager.h>
#include <ResourceManager\ResourceManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <ShaderManager\ShaderManager.h>
#include <Utils/DebugUtils.h>

namespace {
	const std::uint32_t MAX_NUM_CMD_LISTS{ 3U };
}

using namespace DirectX;

tbb::empty_task* MasterRenderTask::Create(MasterRenderTask* &masterRenderTask) {
	tbb::empty_task* parent{ new (tbb::task::allocate_root()) tbb::empty_task };
	parent->set_ref_count(2);
	masterRenderTask = new (parent->allocate_child()) MasterRenderTask();
	return parent;
}

void MasterRenderTask::Init(const HWND hwnd) noexcept {
	mHwnd = hwnd;

	mTimer.Reset();

	InitSystems();

	ResourceManager::gManager->CreateFence(0U, D3D12_FENCE_FLAG_NONE, mFence);
	CreateCommandObjects();	
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

void MasterRenderTask::InitCmdBuilders(Scene* scene) noexcept {
	ASSERT(scene != nullptr);

	scene->GenerateTasks(mCmdListProcessor->CmdListQueue(), mCmdBuilderTasks);
	FlushCommandQueue();
	const std::uint64_t count{ _countof(mFenceByQueuedFrameIndex) };
	for (std::uint64_t i = 0UL; i < count; ++i) {
		mFenceByQueuedFrameIndex[i] = mCurrentFence;
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

		UpdateCamera();
		
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue{ mCmdListProcessor->CmdListQueue() };
		ASSERT(mCmdListProcessor->IsIdle());

		// Begin Frame task + # cmd build tasks
		const std::uint32_t taskCount{ (std::uint32_t)mCmdBuilderTasks.size() + 1U };
		mCmdListProcessor->ResetExecutedTasksCounter();

		ID3D12CommandAllocator* cmdAllocFrameBegin{ mCmdAllocFrameBegin[mCurrQueuedFrameIndex] };
		ID3D12CommandAllocator* cmdAllocFrameEnd{ mCmdAllocFrameEnd[mCurrQueuedFrameIndex] };

		CHECK_HR(cmdAllocFrameBegin->Reset());
		CHECK_HR(mCmdListFrameBegin->Reset(cmdAllocFrameBegin, nullptr));

		mCmdListFrameBegin->RSSetViewports(1U, &Settings::sScreenViewport);
		mCmdListFrameBegin->RSSetScissorRects(1U, &Settings::sScissorRect);

		CD3DX12_RESOURCE_BARRIER resBarrier{ CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET) };
		mCmdListFrameBegin->ResourceBarrier(1, &resBarrier);
		const D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle = CurrentBackBufferView();
		const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = DepthStencilView();
		mCmdListFrameBegin->OMSetRenderTargets(1U, &backBufferHandle, true, &dsvHandle);

		mCmdListFrameBegin->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0U, nullptr);
		mCmdListFrameBegin->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0U, 0U, nullptr);

		// Execute begin Frame task + # cmd build tasks
		CHECK_HR(mCmdListFrameBegin->Close());
		cmdListQueue.push(mCmdListFrameBegin);
		tbb::parallel_for(tbb::blocked_range<std::size_t>(0, taskCount - 1, (taskCount - 1) / Settings::sCpuProcessors),
			[&](const tbb::blocked_range<size_t>& r) {
			for (size_t i = r.begin(); i != r.end(); ++i)
				mCmdBuilderTasks[i]->BuildCommandLists(cmdListQueue, mView, mProj, backBufferHandle, dsvHandle);
		}
		);

		CHECK_HR(cmdAllocFrameEnd->Reset());
		CHECK_HR(mCmdListFrameEnd->Reset(cmdAllocFrameEnd, nullptr));

		resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		mCmdListFrameEnd->ResourceBarrier(1U, &resBarrier);

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

void MasterRenderTask::UpdateCamera() noexcept {
	static std::int32_t lastXY[]{ 0UL, 0UL };
	static const float sCameraOffset{ 10.0f };
	static const float sCameraMultiplier{ 5.0f };

	ASSERT(Keyboard::gKeyboard.get() != nullptr);
	ASSERT(Mouse::gMouse.get() != nullptr);

	if (Camera::gCamera->UpdateViewMatrix()) {
		mProj = Camera::gCamera->GetProj4x4f();
		mView = Camera::gCamera->GetView4x4f();
	}

	// Update camera based on keyboard
	const float offset = sCameraOffset * (Keyboard::gKeyboard->IsKeyDown(DIK_LSHIFT) ? sCameraMultiplier : 1.0f) * mTimer.DeltaTime();
	//const float offset = 0.00005f;
	if (Keyboard::gKeyboard->IsKeyDown(DIK_W)) {
		Camera::gCamera->Walk(offset);
	}
	if (Keyboard::gKeyboard->IsKeyDown(DIK_S)) {
		Camera::gCamera->Walk(-offset);
	}
	if (Keyboard::gKeyboard->IsKeyDown(DIK_A)) {
		Camera::gCamera->Strafe(-offset);
	}
	if (Keyboard::gKeyboard->IsKeyDown(DIK_D)) {
		Camera::gCamera->Strafe(offset);
	}

	// Update camera based on mouse
	const std::int32_t x{ Mouse::gMouse->X() };
	const std::int32_t y{ Mouse::gMouse->Y() };
	if (Mouse::gMouse->IsButtonDown(Mouse::MouseButtonsLeft)) {
		// Make each pixel correspond to a quarter of a degree.
		const float dx{ XMConvertToRadians(0.25f * (float)(x - lastXY[0])) };
		const float dy{ XMConvertToRadians(0.25f * (float)(y - lastXY[1])) };

		Camera::gCamera->Pitch(dy);
		Camera::gCamera->RotateY(dx);
	}

	lastXY[0] = x;
	lastXY[1] = y;
}

void MasterRenderTask::Finalize() noexcept {
	mCmdListProcessor->Terminate();
	mCmdListProcessorParent->wait_for_all();
	FlushCommandQueue();
}

void MasterRenderTask::CreateRtvAndDsv() noexcept {
	ASSERT(D3dData::mDevice != nullptr);
	ASSERT(D3dData::mSwapChain == nullptr);

	CreateRtvAndDsvDescriptorHeaps();

	// Setup RTV descriptor to specify sRGB format.
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = Settings::sRTVFormats[0U];
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// Create swap chain and render target views
	D3dData::CreateSwapChain(mHwnd, *mCmdQueue);
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
	ASSERT(Settings::sQueuedFrameCount > 0U);

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CommandManager::gManager->CreateCmdQueue(queueDesc, mCmdQueue);

	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		CommandManager::gManager->CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAllocFrameBegin[i]);
		CommandManager::gManager->CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAllocFrameEnd[i]);
	}
	CommandManager::gManager->CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *mCmdAllocFrameBegin[0], mCmdListFrameBegin);
	CommandManager::gManager->CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *mCmdAllocFrameEnd[0], mCmdListFrameEnd);

	// Start off in a closed state. This is because the first time we refer 
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

	// Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU time line, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	mFenceByQueuedFrameIndex[mCurrQueuedFrameIndex] = ++mCurrentFence;
	CHECK_HR(mCmdQueue->Signal(mFence, mCurrentFence));
	mCurrQueuedFrameIndex = (mCurrQueuedFrameIndex + 1U) % Settings::sQueuedFrameCount;	

	// If we executed command lists for all queued frames, then we need to wait
	// at least 1 of them to be completed, before continue generating command lists. 
	const std::uint64_t oldestFence{ mFenceByQueuedFrameIndex[mCurrQueuedFrameIndex] };
	if (mFence->GetCompletedValue() < oldestFence) {
		const HANDLE eventHandle{ CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS) };
		ASSERT(eventHandle);

		// Fire event when GPU hits current fence.  
		CHECK_HR(mFence->SetEventOnCompletion(oldestFence, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

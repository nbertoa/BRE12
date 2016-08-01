#include "MasterRender.h"

#include <DirectXColors.h>
#include <tbb/parallel_for.h>

#include <App/Scene.h>
#include <CommandManager/CommandManager.h>
#include <DXUtils/d3dx12.h>
#include <GlobalData/D3dData.h>
#include <GlobalData/Settings.h>
#include <Input/Keyboard.h>
#include <Input/Mouse.h>
#include <RenderTask\CmdListRecorder.h>
#include <ResourceManager\ResourceManager.h>

using namespace DirectX;

namespace {
	const std::uint32_t MAX_NUM_CMD_LISTS{ 3U };
	void UpdateCamera(Camera& camera, XMFLOAT4X4& view, XMFLOAT4X4& proj, const float deltaTime) noexcept {
		static std::int32_t lastXY[]{ 0UL, 0UL };
		static const float sCameraOffset{ 10.0f };
		static const float sCameraMultiplier{ 5.0f };

		if (camera.UpdateViewMatrix()) {
			proj = camera.GetProj4x4f();
			view = camera.GetView4x4f();
		}

		// Update camera based on keyboard
		const float offset = sCameraOffset * (Keyboard::Get().IsKeyDown(DIK_LSHIFT) ? sCameraMultiplier : 1.0f) * deltaTime;
		//const float offset = 0.00005f;
		if (Keyboard::Get().IsKeyDown(DIK_W)) {
			camera.Walk(offset);
		}
		if (Keyboard::Get().IsKeyDown(DIK_S)) {
			camera.Walk(-offset);
		}
		if (Keyboard::Get().IsKeyDown(DIK_A)) {
			camera.Strafe(-offset);
		}
		if (Keyboard::Get().IsKeyDown(DIK_D)) {
			camera.Strafe(offset);
		}

		// Update camera based on mouse
		const std::int32_t x{ Mouse::Get().X() };
		const std::int32_t y{ Mouse::Get().Y() };
		if (Mouse::Get().IsButtonDown(Mouse::MouseButtonsLeft)) {
			// Make each pixel correspond to a quarter of a degree.
			const float dx{ XMConvertToRadians(0.25f * (float)(x - lastXY[0])) };
			const float dy{ XMConvertToRadians(0.25f * (float)(y - lastXY[1])) };

			camera.Pitch(dy);
			camera.RotateY(dx);
		}

		lastXY[0] = x;
		lastXY[1] = y;
	}

	void CreateSwapChain(const HWND hwnd, ID3D12CommandQueue& cmdQueue, Microsoft::WRL::ComPtr<IDXGISwapChain3>& swapChain3) noexcept {
		IDXGISwapChain* swapChain{ nullptr };

		DXGI_SWAP_CHAIN_DESC sd = {};
		sd.BufferDesc.Width = Settings::sWindowWidth;
		sd.BufferDesc.Height = Settings::sWindowHeight;
		sd.BufferDesc.RefreshRate.Numerator = 60U;
		sd.BufferDesc.RefreshRate.Denominator = 1U;
		sd.BufferDesc.Format = MasterRender::BackBufferFormat();
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.SampleDesc.Count = 1U;
		sd.SampleDesc.Quality = 0U;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = Settings::sSwapChainBufferCount;
		sd.OutputWindow = hwnd;
		sd.Windowed = true;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.Flags = 0;

		// Note: Swap chain uses queue to perform flush.		
		CHECK_HR(D3dData::Factory().CreateSwapChain(&cmdQueue, &sd, &swapChain));

		CHECK_HR(swapChain->QueryInterface(IID_PPV_ARGS(swapChain3.GetAddressOf())));

		// Set sRGB color space
		swapChain3->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
		swapChain3->SetFullscreenState(Settings::sFullscreen, nullptr);

		// Resize the swap chain.
		CHECK_HR(swapChain3->ResizeBuffers(Settings::sSwapChainBufferCount, Settings::sWindowWidth, Settings::sWindowHeight, MasterRender::BackBufferFormat(), 0U));
	}
}

using namespace DirectX;

const DXGI_FORMAT MasterRender::sRTVFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB };
const DXGI_FORMAT MasterRender::sGeomPassBufferFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{	
	DXGI_FORMAT_R32G32B32A32_FLOAT,
	DXGI_FORMAT_R32G32B32A32_FLOAT,
	DXGI_FORMAT_R32G32B32A32_FLOAT,
	DXGI_FORMAT_R32G32B32A32_FLOAT,
	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
	DXGI_FORMAT_UNKNOWN,
	DXGI_FORMAT_UNKNOWN,
	DXGI_FORMAT_UNKNOWN
};

MasterRender* MasterRender::Create(const HWND hwnd, ID3D12Device& device, Scene* scene) noexcept {
	ASSERT(scene != nullptr);

	tbb::empty_task* parent{ new (tbb::task::allocate_root()) tbb::empty_task };
	parent->set_ref_count(2);
	return new (parent->allocate_child()) MasterRender(hwnd, device, scene);
}

MasterRender::MasterRender(const HWND hwnd, ID3D12Device& device, Scene* scene)
	: mHwnd(hwnd)
	, mDevice(device)
{
	ResourceManager::Get().CreateFence(0U, D3D12_FENCE_FLAG_NONE, mFence);
	CreateCommandObjects();
	CreateRtvAndDsv();
	CreateGeometryPassRtvs();

	mCamera.SetLens(Settings::sFieldOfView, Settings::AspectRatio(), Settings::sNearPlaneZ, Settings::sFarPlaneZ);

	// Create and spawn command list processor thread.
	mCmdListProcessor = CommandListProcessor::Create(mCmdQueue, MAX_NUM_CMD_LISTS);
	ASSERT(mCmdListProcessor != nullptr);
	
	InitCmdListRecorders(scene);
	parent()->spawn(*this);
}

void MasterRender::InitCmdListRecorders(Scene* scene) noexcept {
	ASSERT(scene != nullptr);

	scene->GenerateGeomPassRecorders(mCmdListProcessor->CmdListQueue(), mCmdListRecorders);
	FlushCommandQueue();
	const std::uint64_t count{ _countof(mFenceByQueuedFrameIndex) };
	for (std::uint64_t i = 0UL; i < count; ++i) {
		mFenceByQueuedFrameIndex[i] = mCurrentFence;
	}
}

void MasterRender::Terminate() noexcept {
	mTerminate = true;
	parent()->wait_for_all();
}

tbb::task* MasterRender::execute() {
	while (!mTerminate) {
		mTimer.Tick();

		UpdateCamera(mCamera, mView, mProj, mTimer.DeltaTime());

		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue{ mCmdListProcessor->CmdListQueue() };
		ASSERT(mCmdListProcessor->IsIdle());

		// Begin Frame task + # cmd build tasks
		const std::uint32_t taskCount{ (std::uint32_t)mCmdListRecorders.size() + 1U };
		mCmdListProcessor->ResetExecutedTasksCounter();

		ID3D12CommandAllocator* cmdAllocFrameBegin{ mCmdAllocFrameBegin[mCurrQueuedFrameIndex] };
		ID3D12CommandAllocator* cmdAllocFrameEnd{ mCmdAllocFrameEnd[mCurrQueuedFrameIndex] };

		CHECK_HR(cmdAllocFrameBegin->Reset());
		CHECK_HR(mCmdListFrameBegin->Reset(cmdAllocFrameBegin, nullptr));

		mCmdListFrameBegin->RSSetViewports(1U, &Settings::sScreenViewport);
		mCmdListFrameBegin->RSSetScissorRects(1U, &Settings::sScissorRect);

		// Set resource barriers and render targets
		CD3DX12_RESOURCE_BARRIER presentToRtBarriers[GEOMBUFFERS_COUNT + 1U]{ 			
			CD3DX12_RESOURCE_BARRIER::Transition(mGeomPassBuffers[NORMAL].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
			CD3DX12_RESOURCE_BARRIER::Transition(mGeomPassBuffers[POSITION].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
			CD3DX12_RESOURCE_BARRIER::Transition(mGeomPassBuffers[BASECOLOR_METALMASK].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
			CD3DX12_RESOURCE_BARRIER::Transition(mGeomPassBuffers[REFLECTANCE_SMOOTHNESS].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
			CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
		};
		mCmdListFrameBegin->ResourceBarrier(_countof(presentToRtBarriers), presentToRtBarriers);
		const D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuDescHandles[GEOMBUFFERS_COUNT + 1U]{			
			mGeomPassBuffersRTVCpuDescHandles[NORMAL],
			mGeomPassBuffersRTVCpuDescHandles[POSITION],
			mGeomPassBuffersRTVCpuDescHandles[BASECOLOR_METALMASK],
			mGeomPassBuffersRTVCpuDescHandles[REFLECTANCE_SMOOTHNESS],
			CurrentBackBufferView(),
		};
		const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = DepthStencilView();
		mCmdListFrameBegin->OMSetRenderTargets(_countof(rtvCpuDescHandles), rtvCpuDescHandles, false, &dsvHandle);
		
		mCmdListFrameBegin->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::Black, 0U, nullptr);
		for (std::uint32_t i = 0U; i < GEOMBUFFERS_COUNT; ++i) {
			mCmdListFrameBegin->ClearRenderTargetView(mGeomPassBuffersRTVCpuDescHandles[i], DirectX::Colors::Black, 0U, nullptr);
		}
		
		mCmdListFrameBegin->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0U, 0U, nullptr);

		// Execute begin Frame task + # cmd build tasks
		CHECK_HR(mCmdListFrameBegin->Close());
		cmdListQueue.push(mCmdListFrameBegin);
		const std::uint32_t grainSize{ max(1U, (taskCount - 1) / Settings::sCpuProcessors) };
		tbb::parallel_for(tbb::blocked_range<std::size_t>(0, taskCount - 1, grainSize),
			[&](const tbb::blocked_range<size_t>& r) {
			for (size_t i = r.begin(); i != r.end(); ++i)
				mCmdListRecorders[i]->RecordCommandLists(mView, mProj, rtvCpuDescHandles, _countof(rtvCpuDescHandles), dsvHandle);
		}
		);

		CHECK_HR(cmdAllocFrameEnd->Reset());
		CHECK_HR(mCmdListFrameEnd->Reset(cmdAllocFrameEnd, nullptr));

		CD3DX12_RESOURCE_BARRIER rtToPresentBarriers[GEOMBUFFERS_COUNT + 1U]{			
			CD3DX12_RESOURCE_BARRIER::Transition(mGeomPassBuffers[NORMAL].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT),
			CD3DX12_RESOURCE_BARRIER::Transition(mGeomPassBuffers[POSITION].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT),
			CD3DX12_RESOURCE_BARRIER::Transition(mGeomPassBuffers[BASECOLOR_METALMASK].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT),
			CD3DX12_RESOURCE_BARRIER::Transition(mGeomPassBuffers[REFLECTANCE_SMOOTHNESS].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT),
			CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT),
		};
		mCmdListFrameEnd->ResourceBarrier(_countof(rtToPresentBarriers), rtToPresentBarriers);

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

	mCmdListProcessor->Terminate();
	FlushCommandQueue();
	return nullptr;
}

void MasterRender::CreateRtvAndDsv() noexcept {
	CreateRtvAndDsvDescriptorHeaps();

	// Setup RTV descriptor to specify sRGB format.
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = sRTVFormats[0U];
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// Create swap chain and render target views
	ASSERT(mSwapChain == nullptr);
	CreateSwapChain(mHwnd, *mCmdQueue, mSwapChain);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	const std::uint32_t rtvDescSize{ mDevice.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) };
	for (std::uint32_t i = 0U; i < Settings::sSwapChainBufferCount; ++i) {
		CHECK_HR(mSwapChain->GetBuffer(i, IID_PPV_ARGS(mSwapChainBuffer[i].GetAddressOf())));
		mDevice.CreateRenderTargetView(mSwapChainBuffer[i].Get(), &rtvDesc, rtvHeapHandle);
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
	depthStencilDesc.Format = sDepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = 1U;
	depthStencilDesc.SampleDesc.Quality = 0U;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = sDepthStencilFormat;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0U;
	CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
	ResourceManager::Get().CreateCommittedResource(heapProps, D3D12_HEAP_FLAG_NONE, depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, clearValue, mDepthStencilBuffer);

	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	mDevice.CreateDepthStencilView(mDepthStencilBuffer, nullptr, DepthStencilView());
}

void MasterRender::CreateRtvAndDsvDescriptorHeaps() noexcept {
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = Settings::sSwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ResourceManager::Get().CreateDescriptorHeap(rtvHeapDesc, mRtvHeap);

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1U;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0U;
	ResourceManager::Get().CreateDescriptorHeap(dsvHeapDesc, mDsvHeap);
}

void MasterRender::CreateGeometryPassRtvs() noexcept {
	// Create geometry pass buffers desc heap
	D3D12_DESCRIPTOR_HEAP_DESC descHeap = {};
	descHeap.NumDescriptors = _countof(mGeomPassBuffers);
	descHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descHeap.NodeMask = 0;
	ResourceManager::Get().CreateDescriptorHeap(descHeap, mGeomPassBuffersRTVDescHeap);

	// Set shared geometry pass buffers properties
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Alignment = 0U;
	resDesc.Width = Settings::sWindowWidth;
	resDesc.Height = Settings::sWindowHeight;
	resDesc.DepthOrArraySize = 1U;
	resDesc.MipLevels = 0U;	
	resDesc.SampleDesc.Count = 1U;
	resDesc.SampleDesc.Quality = 0U;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearValue{};	
	clearValue.Color[0U] = 0.0f;
	clearValue.Color[1U] = 0.0f;
	clearValue.Color[2U] = 0.0f;
	clearValue.Color[3U] = 1.0f;
	mGeomPassBuffers[NORMAL].Reset();
		
	CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
	
	ID3D12Resource* res{ nullptr };
	D3D12_CPU_DESCRIPTOR_HANDLE rtvDescHeapBeginDescHandle(mGeomPassBuffersRTVDescHeap->GetCPUDescriptorHandleForHeapStart());
	const std::size_t rtvDescHandleIncSize{ ResourceManager::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) };

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// Creeate RTV's descriptors
	for (std::uint32_t i = 0U; i < GEOMBUFFERS_COUNT; ++i) {
		resDesc.Format = sGeomPassBufferFormats[i];
		clearValue.Format = resDesc.Format;
		rtvDesc.Format = resDesc.Format;
		ResourceManager::Get().CreateCommittedResource(heapProps, D3D12_HEAP_FLAG_NONE, resDesc, D3D12_RESOURCE_STATE_COMMON, clearValue, res);
		mGeomPassBuffers[i] = Microsoft::WRL::ComPtr<ID3D12Resource>(res);
		mDevice.CreateRenderTargetView(mGeomPassBuffers[i].Get(), &rtvDesc, rtvDescHeapBeginDescHandle);
		mGeomPassBuffersRTVCpuDescHandles[i] = rtvDescHeapBeginDescHandle;
		rtvDescHeapBeginDescHandle.ptr += rtvDescHandleIncSize;
	}
}

void MasterRender::CreateCommandObjects() noexcept {
	ASSERT(Settings::sQueuedFrameCount > 0U);

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CommandManager::Get().CreateCmdQueue(queueDesc, mCmdQueue);

	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAllocFrameBegin[i]);
		CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAllocFrameEnd[i]);
	}
	CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *mCmdAllocFrameBegin[0], mCmdListFrameBegin);
	CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *mCmdAllocFrameEnd[0], mCmdListFrameEnd);

	// Start off in a closed state. This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	mCmdListFrameBegin->Close();
	mCmdListFrameEnd->Close();
}

ID3D12Resource* MasterRender::CurrentBackBuffer() const noexcept {
	ASSERT(mSwapChain != nullptr);
	const std::uint32_t currBackBuffer{ mSwapChain->GetCurrentBackBufferIndex() };
	return mSwapChainBuffer[currBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE MasterRender::CurrentBackBufferView() const noexcept {
	const std::uint32_t rtvDescSize{ mDevice.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) };
	const std::uint32_t currBackBuffer{ mSwapChain->GetCurrentBackBufferIndex() };
	return D3D12_CPU_DESCRIPTOR_HANDLE{ mRtvHeap->GetCPUDescriptorHandleForHeapStart().ptr + currBackBuffer * rtvDescSize };
}

D3D12_CPU_DESCRIPTOR_HANDLE MasterRender::DepthStencilView() const noexcept {
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void MasterRender::FlushCommandQueue() noexcept {
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

void MasterRender::SignalFenceAndPresent() noexcept {
	ASSERT(mSwapChain != nullptr);
	CHECK_HR(mSwapChain->Present(1U, 0U));

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

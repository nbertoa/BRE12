#include "MasterRender.h"

#include <DirectXColors.h>
#include <tbb/parallel_for.h>

#include <CommandManager/CommandManager.h>
#include <DXUtils/d3dx12.h>
#include <GlobalData/D3dData.h>
#include <GlobalData/Settings.h>
#include <Input/Keyboard.h>
#include <Input/Mouse.h>
#include <ResourceManager\ResourceManager.h>
#include <Scene/CmdListRecorder.h>
#include <Scene/Scene.h>

// Uncomment to use Vsync. We do not use a boolean to avoid branches
//#define V_SYNC

using namespace DirectX;

namespace {
	const std::uint32_t MAX_NUM_CMD_LISTS{ 3U };
	void UpdateCamera(Camera& camera, XMFLOAT4X4& view, XMFLOAT4X4& proj, const float deltaTime) noexcept {
		static std::int32_t lastXY[]{ 0UL, 0UL };
		static const float sCameraOffset{ 10.0f };
		static const float sCameraMultiplier{ 20.0f };

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
		IDXGISwapChain1* baseSwapChain{ nullptr };

		DXGI_SWAP_CHAIN_DESC1 sd = {};
		sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		sd.BufferCount = Settings::sSwapChainBufferCount;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
#ifdef V_SYNC
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
#else 
		sd.Flags = 0U;
#endif
		sd.Format = MasterRender::BackBufferFormat();
		sd.SampleDesc.Count = 1U;
		sd.Scaling = DXGI_SCALING_NONE;
		sd.Stereo = false;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		CHECK_HR(D3dData::Factory().CreateSwapChainForHwnd(&cmdQueue, hwnd, &sd, nullptr, nullptr, &baseSwapChain));
		CHECK_HR(baseSwapChain->QueryInterface(IID_PPV_ARGS(swapChain3.GetAddressOf())));

		CHECK_HR(swapChain3->ResizeBuffers(Settings::sSwapChainBufferCount, Settings::sWindowWidth, Settings::sWindowHeight, MasterRender::BackBufferFormat(), sd.Flags));

		// Set sRGB color space
		swapChain3->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);

		// Make window association
		CHECK_HR(D3dData::Factory().MakeWindowAssociation(hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_PRINT_SCREEN));

#ifdef V_SYNC
		CHECK_HR(swapChain3->SetMaximumFrameLatency(Settings::sQueuedFrameCount));
#endif
	}
}

using namespace DirectX;

const DXGI_FORMAT MasterRender::sGeomPassBufferFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{	
	DXGI_FORMAT_R16G16_FLOAT,	
	DXGI_FORMAT_R8G8B8A8_UNORM,
	DXGI_FORMAT_R8G8B8A8_UNORM,
	DXGI_FORMAT_R16_UNORM,
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

	scene->GenerateGeomPassRecorders(mCmdListProcessor->CmdListQueue(), mGeomPassCmdListRecorders);
	FlushCommandQueue();

	scene->GenerateLightPassRecorders(mCmdListProcessor->CmdListQueue(), mGeomPassBuffers, GEOMBUFFERS_COUNT, mLightPassCmdListRecorders);
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
		ASSERT(mCmdListProcessor->IsIdle());

		BeginFrameTask();
		MiddleFrameTask();
		EndFrameTask();

		SignalFenceAndPresent();
	}

	mCmdListProcessor->Terminate();
	FlushCommandQueue();
	return nullptr;
}

void MasterRender::BeginFrameTask() {
	std::uint32_t taskCount{ (std::uint32_t)mGeomPassCmdListRecorders.size() };
	mCmdListProcessor->ResetExecutedTasksCounter();

	ID3D12CommandAllocator* cmdAllocFrameBegin{ mCmdAllocFrameBegin[mCurrQueuedFrameIndex] };

	CHECK_HR(cmdAllocFrameBegin->Reset());
	CHECK_HR(mCmdListFrameBegin->Reset(cmdAllocFrameBegin, nullptr));

	mCmdListFrameBegin->RSSetViewports(1U, &Settings::sScreenViewport);
	mCmdListFrameBegin->RSSetScissorRects(1U, &Settings::sScissorRect);

	// Set resource barriers and render targets
	CD3DX12_RESOURCE_BARRIER presentToRtBarriers[GEOMBUFFERS_COUNT + 1U]{
		CD3DX12_RESOURCE_BARRIER::Transition(mGeomPassBuffers[NORMAL].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),		
		CD3DX12_RESOURCE_BARRIER::Transition(mGeomPassBuffers[BASECOLOR_METALMASK].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
		CD3DX12_RESOURCE_BARRIER::Transition(mGeomPassBuffers[REFLECTANCE_SMOOTHNESS].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
		CD3DX12_RESOURCE_BARRIER::Transition(mGeomPassBuffers[DEPTH].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
		CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
	};
	mCmdListFrameBegin->ResourceBarrier(_countof(presentToRtBarriers), presentToRtBarriers);
	const D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuDescHandles[GEOMBUFFERS_COUNT]{
		mGeomPassBuffersRTVCpuDescHandles[NORMAL],		
		mGeomPassBuffersRTVCpuDescHandles[BASECOLOR_METALMASK],
		mGeomPassBuffersRTVCpuDescHandles[REFLECTANCE_SMOOTHNESS],
		mGeomPassBuffersRTVCpuDescHandles[DEPTH]
	};
	const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = DepthStencilView();
	mCmdListFrameBegin->OMSetRenderTargets(_countof(rtvCpuDescHandles), rtvCpuDescHandles, false, &dsvHandle);

	mCmdListFrameBegin->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::Black, 0U, nullptr);
	for (std::uint32_t i = 0U; i < GEOMBUFFERS_COUNT; ++i) {
		mCmdListFrameBegin->ClearRenderTargetView(mGeomPassBuffersRTVCpuDescHandles[i], DirectX::Colors::Black, 0U, nullptr);
	}

	mCmdListFrameBegin->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0U, 0U, nullptr);
	CHECK_HR(mCmdListFrameBegin->Close());

	// Execute begin Frame task + # cmd build tasks		
	{
		ID3D12CommandList* cmdLists[] = { mCmdListFrameBegin };
		mCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
	}

	std::uint32_t grainSize{ max(1U, (taskCount) / Settings::sCpuProcessors) };
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, taskCount, grainSize),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
			mGeomPassCmdListRecorders[i]->RecordCommandLists(mView, mProj, rtvCpuDescHandles, _countof(rtvCpuDescHandles), dsvHandle);
	}
	);

	// Wait until all previous tasks command lists are executed
	while (mCmdListProcessor->ExecutedTasksCounter() < taskCount) {
		Sleep(0U);
	}
}

void MasterRender::MiddleFrameTask() {
	ID3D12CommandAllocator* cmdAllocFrameMiddle{ mCmdAllocFrameMiddle[mCurrQueuedFrameIndex] };

	CHECK_HR(cmdAllocFrameMiddle->Reset());
	CHECK_HR(mCmdListFrameMiddle->Reset(cmdAllocFrameMiddle, nullptr));

	CD3DX12_RESOURCE_BARRIER rtToSrvBarriers[GEOMBUFFERS_COUNT]{
		CD3DX12_RESOURCE_BARRIER::Transition(mGeomPassBuffers[NORMAL].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),		
		CD3DX12_RESOURCE_BARRIER::Transition(mGeomPassBuffers[BASECOLOR_METALMASK].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(mGeomPassBuffers[REFLECTANCE_SMOOTHNESS].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(mGeomPassBuffers[DEPTH].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
	};
	mCmdListFrameMiddle->ResourceBarrier(_countof(rtToSrvBarriers), rtToSrvBarriers);

	CHECK_HR(mCmdListFrameMiddle->Close());

	// Middle Frame task + # light pass cmd list recorders
	const std::uint32_t taskCount((std::uint32_t)mLightPassCmdListRecorders.size());
	mCmdListProcessor->ResetExecutedTasksCounter();

	// Execute middle frame task
	{
		ID3D12CommandList* cmdLists[] = { mCmdListFrameMiddle };
		mCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
	}
	const std::uint32_t grainSize(max(1U, taskCount / Settings::sCpuProcessors));
	D3D12_CPU_DESCRIPTOR_HANDLE currBackBuffer(CurrentBackBufferView());
	const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = DepthStencilView();
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, taskCount, grainSize),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
			mLightPassCmdListRecorders[i]->RecordCommandLists(mView, mProj, &currBackBuffer, 1U, dsvHandle);
	}
	);

	// Wait until all previous tasks command lists are executed
	while (mCmdListProcessor->ExecutedTasksCounter() < taskCount) {
		Sleep(0U);
	}
}

void MasterRender::EndFrameTask() {
	ID3D12CommandAllocator* cmdAllocFrameEnd{ mCmdAllocFrameEnd[mCurrQueuedFrameIndex] };

	CHECK_HR(cmdAllocFrameEnd->Reset());
	CHECK_HR(mCmdListFrameEnd->Reset(cmdAllocFrameEnd, nullptr));

	CD3DX12_RESOURCE_BARRIER rtToPresentBarriers[GEOMBUFFERS_COUNT + 1U]{
		CD3DX12_RESOURCE_BARRIER::Transition(mGeomPassBuffers[NORMAL].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PRESENT),		
		CD3DX12_RESOURCE_BARRIER::Transition(mGeomPassBuffers[BASECOLOR_METALMASK].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PRESENT),
		CD3DX12_RESOURCE_BARRIER::Transition(mGeomPassBuffers[REFLECTANCE_SMOOTHNESS].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PRESENT),
		CD3DX12_RESOURCE_BARRIER::Transition(mGeomPassBuffers[DEPTH].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PRESENT),
		CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT),
	};
	mCmdListFrameEnd->ResourceBarrier(_countof(rtToPresentBarriers), rtToPresentBarriers);

	CHECK_HR(mCmdListFrameEnd->Close());
	{
		ID3D12CommandList* cmdLists[] = { mCmdListFrameEnd };
		mCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
	}
}

void MasterRender::CreateRtvAndDsv() noexcept {
	CreateRtvAndDsvDescriptorHeaps();

	// Setup RTV descriptor to specify sRGB format.
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = sBackBufferRTFormat;
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
	ResourceManager::Get().CreateCommittedResource(heapProps, D3D12_HEAP_FLAG_NONE, depthStencilDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, clearValue, mDepthStencilBuffer);

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

	// Create RTV's descriptors
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
		CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAllocFrameMiddle[i]);
		CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAllocFrameEnd[i]);
	}
	CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *mCmdAllocFrameBegin[0], mCmdListFrameBegin);
	CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *mCmdAllocFrameMiddle[0], mCmdListFrameMiddle);
	CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *mCmdAllocFrameEnd[0], mCmdListFrameEnd);

	// Start off in a closed state. This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	mCmdListFrameBegin->Close();
	mCmdListFrameMiddle->Close();
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

ID3D12Resource* MasterRender::DepthStencilBuffer() const noexcept {
	ASSERT(mDepthStencilBuffer != nullptr);
	return mDepthStencilBuffer;
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

#ifdef V_SYNC
	static const HANDLE frameLatencyWaitableObj(mSwapChain->GetFrameLatencyWaitableObject());
	WaitForSingleObjectEx(frameLatencyWaitableObj, INFINITE, true);
	CHECK_HR(mSwapChain->Present(1U, 0U));
#else
	CHECK_HR(mSwapChain->Present(0U, 0U));
#endif
	

	// Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU time line, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	mFenceByQueuedFrameIndex[mCurrQueuedFrameIndex] = ++mCurrentFence;
	CHECK_HR(mCmdQueue->Signal(mFence, mCurrentFence));
	mCurrQueuedFrameIndex = (mCurrQueuedFrameIndex + 1U) % Settings::sQueuedFrameCount;	

	// If we executed command lists for all queued frames, then we need to wait
	// at least 1 of them to be completed, before continue recording command lists. 
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

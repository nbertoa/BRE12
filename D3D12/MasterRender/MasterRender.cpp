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
#include <ModelManager\Model.h>
#include <ModelManager\ModelManager.h>
#include <Scene/Scene.h>

using namespace DirectX;

namespace {
	const std::uint32_t MAX_NUM_CMD_LISTS{ 3U };
	void UpdateCamera(Camera& camera, XMFLOAT4X4& viewTranspose, XMFLOAT4X4& projTranpose, XMFLOAT3& eyePosW, const float deltaTime) noexcept {
		static std::int32_t lastXY[]{ 0UL, 0UL };
		static const float sCameraOffset{ 7.5f };
		static const float sCameraMultiplier{ 10.0f };

		if (camera.UpdateViewMatrix()) {
			DirectX::XMStoreFloat4x4(&viewTranspose, MathUtils::GetTranspose(camera.GetView4x4f()));
			DirectX::XMStoreFloat4x4(&projTranpose, MathUtils::GetTranspose(camera.GetProj4x4f()));
			eyePosW = camera.GetPosition3f();
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
		sd.Format = MasterRender::FrameBufferFormat();
		sd.SampleDesc.Count = 1U;
		sd.Scaling = DXGI_SCALING_NONE;
		sd.Stereo = false;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		CHECK_HR(D3dData::Factory().CreateSwapChainForHwnd(&cmdQueue, hwnd, &sd, nullptr, nullptr, &baseSwapChain));
		CHECK_HR(baseSwapChain->QueryInterface(IID_PPV_ARGS(swapChain3.GetAddressOf())));

		CHECK_HR(swapChain3->ResizeBuffers(Settings::sSwapChainBufferCount, Settings::sWindowWidth, Settings::sWindowHeight, MasterRender::FrameBufferFormat(), sd.Flags));

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
	CreateExtraBuffersRtvs();

	mCamera.SetLens(Settings::sFieldOfView, Settings::AspectRatio(), Settings::sNearPlaneZ, Settings::sFarPlaneZ);

	// Create and spawn command list processor thread.
	mCmdListProcessor = CommandListProcessor::Create(mCmdQueue, MAX_NUM_CMD_LISTS);
	ASSERT(mCmdListProcessor != nullptr);
	
	InitCmdListRecorders(scene);
	parent()->spawn(*this);
}

void MasterRender::InitCmdListRecorders(Scene* scene) noexcept {
	ASSERT(scene != nullptr);

	CmdListHelper cmdListHelper(*mCmdQueue, *mFence, mCurrentFence, *mCmdList);

	cmdListHelper.Reset(*mCmdAllocs[0U]);
	scene->GenerateGeomPassRecorders(mCmdListProcessor->CmdListQueue(), cmdListHelper, mGeometryPass.GetRecorders());
	mGeometryPass.Init(mDevice, DepthStencilView());

	cmdListHelper.Reset(*mCmdAllocs[0U]);
	scene->GenerateLightPassRecorders(mCmdListProcessor->CmdListQueue(), mGeometryPass.GetBuffers(), GeometryPass::BUFFERS_COUNT, cmdListHelper, mLightPass.GetRecorders());
	mLightPass.Init(mGeometryPass.GetBuffers(), mColorBufferRTVCpuDescHandle, DepthStencilView());

	cmdListHelper.Reset(*mCmdAllocs[0U]);
	scene->GenerateSkyBoxRecorder(mCmdListProcessor->CmdListQueue(), cmdListHelper, mSkyBoxPass.GetRecorder());
	mSkyBoxPass.Init(mColorBufferRTVCpuDescHandle, DepthStencilView());

	cmdListHelper.Reset(*mCmdAllocs[0U]);
	mToneMappingPass.Init(mDevice, cmdListHelper, mCmdListProcessor->CmdListQueue(), *mColorBuffer.Get(), DepthStencilView());

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

		UpdateCamera(mCamera, mFrameCBuffer.mView, mFrameCBuffer.mProj, mFrameCBuffer.mEyePosW, mTimer.DeltaTime());
		ASSERT(mCmdListProcessor->IsIdle());

		mGeometryPass.Execute(*mCmdListProcessor, *mCmdQueue, mFrameCBuffer);
		mLightPass.Execute(*mCmdListProcessor, *mCmdQueue, mFrameCBuffer);
		mSkyBoxPass.Execute(*mCmdListProcessor, mFrameCBuffer);
		mToneMappingPass.Execute(*mCmdListProcessor, *mCmdQueue, *CurrentFrameBuffer(), CurrentFrameBufferView());
		MergeTask();

		SignalFenceAndPresent();
	}

	mCmdListProcessor->Terminate();
	FlushCommandQueue();
	return nullptr;
}

void MasterRender::MergeTask() {
	ID3D12CommandAllocator* cmdAlloc{ mCmdAllocs[mCurrQueuedFrameIndex] };

	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCmdList->Reset(cmdAlloc, nullptr));

	CD3DX12_RESOURCE_BARRIER rtToPresentBarriers[GeometryPass::BUFFERS_COUNT + 2U]{
		CD3DX12_RESOURCE_BARRIER::Transition(mGeometryPass.GetBuffers()[GeometryPass::NORMAL_SMOOTHNESS_DEPTH].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
		CD3DX12_RESOURCE_BARRIER::Transition(mGeometryPass.GetBuffers()[GeometryPass::BASECOLOR_METALMASK].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
		CD3DX12_RESOURCE_BARRIER::Transition(mGeometryPass.GetBuffers()[GeometryPass::SPECULARREFLECTION].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
		CD3DX12_RESOURCE_BARRIER::Transition(CurrentFrameBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT),
		CD3DX12_RESOURCE_BARRIER::Transition(mColorBuffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
	};
	mCmdList->ResourceBarrier(_countof(rtToPresentBarriers), rtToPresentBarriers);

	CHECK_HR(mCmdList->Close());
	{
		ID3D12CommandList* cmdLists[] = { mCmdList };
		mCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
	}
}

void MasterRender::CreateRtvAndDsv() noexcept {
	CreateRtvAndDsvDescriptorHeaps();

	// Setup RTV descriptor to specify sRGB format.
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = sFrameBufferRTFormat;
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

void MasterRender::CreateExtraBuffersRtvs() noexcept {
	// Create geometry pass buffers desc heap
	D3D12_DESCRIPTOR_HEAP_DESC descHeap = {};
	descHeap.NumDescriptors = 1U;
	descHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descHeap.NodeMask = 0;
	ResourceManager::Get().CreateDescriptorHeap(descHeap, mBuffersRTVDescHeap);

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

	mColorBuffer.Reset();
		
	CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
	
	ID3D12Resource* res{ nullptr };
	D3D12_CPU_DESCRIPTOR_HANDLE rtvDescHeapBeginDescHandle(mBuffersRTVDescHeap->GetCPUDescriptorHandleForHeapStart());
	const std::size_t rtvDescHandleIncSize{ ResourceManager::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) };
	
	// Create RTV's descriptor for color buffer
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	resDesc.Format = sColorBufferFormat;
	D3D12_CLEAR_VALUE clearValue = { resDesc.Format, 0.0f, 0.0f, 0.0f, 1.0f };
	rtvDesc.Format = resDesc.Format;
	ResourceManager::Get().CreateCommittedResource(heapProps, D3D12_HEAP_FLAG_NONE, resDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, clearValue, res);
	mColorBuffer = Microsoft::WRL::ComPtr<ID3D12Resource>(res);
	mDevice.CreateRenderTargetView(mColorBuffer.Get(), &rtvDesc, rtvDescHeapBeginDescHandle);
	mColorBufferRTVCpuDescHandle = rtvDescHeapBeginDescHandle;
}

void MasterRender::CreateCommandObjects() noexcept {
	ASSERT(Settings::sQueuedFrameCount > 0U);

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CommandManager::Get().CreateCmdQueue(queueDesc, mCmdQueue);

	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAllocs[i]);
	}
	CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *mCmdAllocs[0], mCmdList);

	// Start off in a closed state. This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	mCmdList->Close();
}

ID3D12Resource* MasterRender::CurrentFrameBuffer() const noexcept {
	ASSERT(mSwapChain != nullptr);
	const std::uint32_t currBackBuffer{ mSwapChain->GetCurrentBackBufferIndex() };
	return mSwapChainBuffer[currBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE MasterRender::CurrentFrameBufferView() const noexcept {
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

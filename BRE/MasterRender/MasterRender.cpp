#include "MasterRender.h"

#include <tbb/parallel_for.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <CommandManager/CommandManager.h>
#include <DescriptorManager\DescriptorManager.h>
#include <DXUtils/d3dx12.h>
#include <GlobalData/D3dData.h>
#include <GlobalData/Settings.h>
#include <Input/Keyboard.h>
#include <Input/Mouse.h>
#include <ResourceManager\ResourceManager.h>
#include <Scene/Scene.h>

using namespace DirectX;

namespace {
	const std::uint32_t MAX_NUM_CMD_LISTS{ 3U };
	const DXGI_FORMAT sFrameBufferFormat{ DXGI_FORMAT_R8G8B8A8_UNORM };
	
	// Update camera's view matrix and store data in parameters.
	void UpdateCamera(
		Camera& camera,
		const float deltaTime,
		FrameCBuffer& frameCBuffer) noexcept {

		static std::int32_t lastXY[]{ 0UL, 0UL };
		static const float sCameraOffset{ 7.5f };
		static const float sCameraMultiplier{ 10.0f };

		if (camera.UpdateViewMatrix()) {
			DirectX::XMStoreFloat4x4(&frameCBuffer.mView, MathUtils::GetTranspose(camera.GetView4x4f()));
			DirectX::XMFLOAT4X4 inverse;
			camera.GetInvView4x4f(inverse);
			DirectX::XMStoreFloat4x4(&frameCBuffer.mInvView, MathUtils::GetTranspose(inverse));

			DirectX::XMStoreFloat4x4(&frameCBuffer.mProj, MathUtils::GetTranspose(camera.GetProj4x4f()));
			camera.GetInvProj4x4f(inverse);
			DirectX::XMStoreFloat4x4(&frameCBuffer.mInvProj, MathUtils::GetTranspose(inverse));

			frameCBuffer.mEyePosW = camera.GetPosition4f();
		}

		// Update camera based on keyboard
		const float offset = sCameraOffset * (Keyboard::Get().IsKeyDown(DIK_LSHIFT) ? sCameraMultiplier : 1.0f) * deltaTime;
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
			const float dx{ XMConvertToRadians(0.25f * static_cast<float>(x - lastXY[0])) };
			const float dy{ XMConvertToRadians(0.25f * static_cast<float>(y - lastXY[1])) };

			camera.Pitch(dy);
			camera.RotateY(dx);
		}

		lastXY[0] = x;
		lastXY[1] = y;
	}

	// Create swap chainm and stores it in swapChain3 parameter.
	void CreateSwapChain(
		const HWND hwnd, 
		ID3D12CommandQueue& cmdQueue, 
		const DXGI_FORMAT frameBufferFormat,
		Microsoft::WRL::ComPtr<IDXGISwapChain3>& swapChain3) noexcept {

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
		sd.Format = frameBufferFormat;
		sd.SampleDesc.Count = 1U;
		sd.Scaling = DXGI_SCALING_NONE;
		sd.Stereo = false;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		CHECK_HR(D3dData::Factory().CreateSwapChainForHwnd(&cmdQueue, hwnd, &sd, nullptr, nullptr, &baseSwapChain));
		CHECK_HR(baseSwapChain->QueryInterface(IID_PPV_ARGS(swapChain3.GetAddressOf())));

		CHECK_HR(swapChain3->ResizeBuffers(Settings::sSwapChainBufferCount, Settings::sWindowWidth, Settings::sWindowHeight, frameBufferFormat, sd.Flags));

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
	// Reference count is 2: 1 parent task + 1 master render task
	parent->set_ref_count(2);
	return new (parent->allocate_child()) MasterRender(hwnd, device, scene);
}

MasterRender::MasterRender(const HWND hwnd, ID3D12Device& device, Scene* scene)
	: mHwnd(hwnd)
	, mDevice(device)
{
	ResourceManager::Get().CreateFence(0U, D3D12_FENCE_FLAG_NONE, mFence);
	CreateMergePassCommandObjects();
	CreateRtvAndDsv();
	CreateColorBuffer();

	mCamera.SetLens(Settings::sFieldOfView, Settings::AspectRatio(), Settings::sNearPlaneZ, Settings::sFarPlaneZ);
	
	// Create and spawn command list processor thread.
	mCmdListExecutor = CommandListExecutor::Create(mCmdQueue, MAX_NUM_CMD_LISTS);
	ASSERT(mCmdListExecutor != nullptr);
	
	InitPasses(scene);

	// Spawns master render task
	parent()->spawn(*this);
}

void MasterRender::InitPasses(Scene* scene) noexcept {
	ASSERT(scene != nullptr);

	// Initialize scene
	scene->Init(*mCmdQueue);
	
	// Generate recorders for all the passes
	scene->GenerateGeomPassRecorders(mGeometryPass.GetRecorders());
	mGeometryPass.Init(DepthStencilCpuDesc(), *mCmdListExecutor, *mCmdQueue);

	ID3D12Resource* skyBoxCubeMap;
	ID3D12Resource* diffuseIrradianceCubeMap;
	ID3D12Resource* specularPreConvolvedCubeMap;
	scene->GenerateCubeMaps(skyBoxCubeMap, diffuseIrradianceCubeMap, specularPreConvolvedCubeMap);
	ASSERT(skyBoxCubeMap != nullptr);
	ASSERT(diffuseIrradianceCubeMap != nullptr);
	ASSERT(specularPreConvolvedCubeMap != nullptr);

	scene->GenerateLightingPassRecorders(
		mGeometryPass.GetBuffers(), 
		GeometryPass::BUFFERS_COUNT, 
		*mDepthStencilBuffer, 
		mLightingPass.GetRecorders());

	mLightingPass.Init(
		mDevice, 
		*mCmdListExecutor,
		*mCmdQueue, 
		mGeometryPass.GetBuffers(),
		GeometryPass::BUFFERS_COUNT,
		*mDepthStencilBuffer,
		mColorBufferRTVCpuDescHandle, 
		DepthStencilCpuDesc(),
		*diffuseIrradianceCubeMap,
		*specularPreConvolvedCubeMap);

	mSkyBoxPass.Init(
		mDevice, 
		*mCmdListExecutor, 
		*mCmdQueue, 
		*skyBoxCubeMap, 
		mColorBufferRTVCpuDescHandle, 
		DepthStencilCpuDesc());

	mToneMappingPass.Init(
		mDevice, 
		*mCmdListExecutor,
		*mCmdQueue, 
		*mColorBuffer.Get(), 
		DepthStencilCpuDesc());
		
	// Initialize fence values for all frames to the same number.
	const std::uint64_t count{ _countof(mFenceValueByQueuedFrameIndex) };
	for (std::uint64_t i = 0UL; i < count; ++i) {
		mFenceValueByQueuedFrameIndex[i] = mCurrFenceValue;
	}
}

void MasterRender::Terminate() noexcept {
	mTerminate = true;
	parent()->wait_for_all();
}

tbb::task* MasterRender::execute() {
	while (!mTerminate) {
		mTimer.Tick();
		UpdateCamera(mCamera, mTimer.DeltaTime(), mFrameCBuffer);

		ASSERT(mCmdListExecutor->IsIdle());

		// Execute passes
		mGeometryPass.Execute(mFrameCBuffer);
		mLightingPass.Execute(mFrameCBuffer);
		mSkyBoxPass.Execute(mFrameCBuffer);
		mToneMappingPass.Execute(*CurrentFrameBuffer(), CurrentFrameBufferCpuDesc());
		ExecuteMergePass();

		SignalFenceAndPresent();
	}

	// If we need to terminate, then we terminates command list processor
	// and waits until all GPU command lists are properly executed.
	mCmdListExecutor->Terminate();
	FlushCommandQueue();

	return nullptr;
}

void MasterRender::ExecuteMergePass() {
	ID3D12CommandAllocator* cmdAlloc{ mMergePassCmdAllocs[mCurrQueuedFrameIndex] };

	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mMergePassCmdList->Reset(cmdAlloc, nullptr));

	// Set barriers
	CD3DX12_RESOURCE_BARRIER barriers[]{
		CD3DX12_RESOURCE_BARRIER::Transition(mGeometryPass.GetBuffers()[GeometryPass::NORMAL_SMOOTHNESS].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
		CD3DX12_RESOURCE_BARRIER::Transition(mGeometryPass.GetBuffers()[GeometryPass::BASECOLOR_METALMASK].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
		CD3DX12_RESOURCE_BARRIER::Transition(CurrentFrameBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT),
		CD3DX12_RESOURCE_BARRIER::Transition(mColorBuffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
	};
	const std::size_t barriersCount = _countof(barriers);
	ASSERT(barriersCount == GeometryPass::BUFFERS_COUNT + 2UL);
	mMergePassCmdList->ResourceBarrier(_countof(barriers), barriers);

	// Execute command list
	CHECK_HR(mMergePassCmdList->Close());
	ID3D12CommandList* cmdLists[] = { mMergePassCmdList };
	mCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
}

void MasterRender::CreateRtvAndDsv() noexcept {
	// Setup RTV descriptor to specify sRGB format.
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = Settings::sFrameBufferRTFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// Create swap chain and render target views
	ASSERT(mSwapChain == nullptr);
	CreateSwapChain(mHwnd, *mCmdQueue, sFrameBufferFormat, mSwapChain);
	const std::uint32_t rtvDescSize{ mDevice.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) };
	for (std::uint32_t i = 0U; i < Settings::sSwapChainBufferCount; ++i) {
		CHECK_HR(mSwapChain->GetBuffer(i, IID_PPV_ARGS(mFrameBuffers[i].GetAddressOf())));
		DescriptorManager::Get().CreateRenderTargetView(*mFrameBuffers[i].Get(), rtvDesc, &mFrameBufferRTVs[i]);
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

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = Settings::sDepthStencilViewFormat;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0U;

	CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
	ResourceManager::Get().CreateCommittedResource(heapProps, D3D12_HEAP_FLAG_NONE, depthStencilDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, mDepthStencilBuffer);

	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
	depthStencilViewDesc.Format = Settings::sDepthStencilViewFormat;
	depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	DescriptorManager::Get().CreateDepthStencilView(*mDepthStencilBuffer, depthStencilViewDesc, &mDepthStencilBufferRTV);
}

void MasterRender::CreateColorBuffer() noexcept {
	// Fill resource description
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
	resDesc.Format = Settings::sColorBufferFormat;

	D3D12_CLEAR_VALUE clearValue = { resDesc.Format, 0.0f, 0.0f, 0.0f, 1.0f };

	mColorBuffer.Reset();
			
	ID3D12Resource* res{ nullptr };
	
	// Create RTV's descriptor
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;		
	rtvDesc.Format = resDesc.Format;
	CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
	ResourceManager::Get().CreateCommittedResource(heapProps, D3D12_HEAP_FLAG_NONE, resDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, res);

	mColorBuffer = Microsoft::WRL::ComPtr<ID3D12Resource>(res);
	DescriptorManager::Get().CreateRenderTargetView(*mColorBuffer.Get(), rtvDesc, &mColorBufferRTVCpuDescHandle);
}

void MasterRender::CreateMergePassCommandObjects() noexcept {
	ASSERT(Settings::sQueuedFrameCount > 0U);

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CommandManager::Get().CreateCmdQueue(queueDesc, mCmdQueue);

	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, mMergePassCmdAllocs[i]);
	}
	CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *mMergePassCmdAllocs[0], mMergePassCmdList);
	mMergePassCmdList->Close();
}

ID3D12Resource* MasterRender::CurrentFrameBuffer() const noexcept {
	ASSERT(mSwapChain != nullptr);
	return mFrameBuffers[mSwapChain->GetCurrentBackBufferIndex()].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE MasterRender::CurrentFrameBufferCpuDesc() const noexcept {
	return mFrameBufferRTVs[mSwapChain->GetCurrentBackBufferIndex()];
}

D3D12_CPU_DESCRIPTOR_HANDLE MasterRender::DepthStencilCpuDesc() const noexcept {
	return mDepthStencilBufferRTV;
}

void MasterRender::FlushCommandQueue() noexcept {
	// Signal a new fence value
	++mCurrFenceValue;
	CHECK_HR(mCmdQueue->Signal(mFence, mCurrFenceValue));

	// Wait until the GPU has completed commands up to this fence point.
	if (mFence->GetCompletedValue() < mCurrFenceValue) {
		const HANDLE eventHandle{ CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS) };
		ASSERT(eventHandle);

		// Fire event when GPU hits current fence.  
		CHECK_HR(mFence->SetEventOnCompletion(mCurrFenceValue, eventHandle));

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
	mFenceValueByQueuedFrameIndex[mCurrQueuedFrameIndex] = ++mCurrFenceValue;
	CHECK_HR(mCmdQueue->Signal(mFence, mCurrFenceValue));
	mCurrQueuedFrameIndex = (mCurrQueuedFrameIndex + 1U) % Settings::sQueuedFrameCount;	

	// If we executed command lists for all queued frames, then we need to wait
	// at least 1 of them to be completed, before continue recording command lists. 
	const std::uint64_t oldestFence{ mFenceValueByQueuedFrameIndex[mCurrQueuedFrameIndex] };
	if (mFence->GetCompletedValue() < oldestFence) {
		const HANDLE eventHandle{ CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS) };
		ASSERT(eventHandle);

		// Fire event when GPU hits current fence.  
		CHECK_HR(mFence->SetEventOnCompletion(oldestFence, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

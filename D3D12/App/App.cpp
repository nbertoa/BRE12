#include "App.h"

#include <vector>
#include <WindowsX.h>

#include <Camera/Camera.h>
#include <DXUtils\d3dx12.h>
#include <Input/Keyboard.h>
#include <MathUtils/MathHelper.h>
#include <PSOManager\PSOManager.h>
#include <ResourceManager\ResourceManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <ShaderManager\ShaderManager.h>
#include <Utils\DebugUtils.h>

App* App::mApp = nullptr;

LRESULT CALLBACK
MainWndProc(HWND hwnd, const std::uint32_t msg, WPARAM wParam, LPARAM lParam) {
	return App::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

App::App(HINSTANCE hInstance)
#ifdef _DEBUG
	: mTaskSchedulerInit()
#else 
	: mTaskSchedulerInit()
#endif
	, mAppInst(hInstance)
{
	ASSERT(mApp == nullptr);
	mApp = this;
}

App::~App() {
	if (mD3dDevice != nullptr) {
		FlushCommandQueue();
	}

	mTaskSchedulerInit.terminate();
}

int32_t App::Run() noexcept {
	ASSERT(Keyboard::gKeyboard.get() != nullptr);

	MSG msg{0U};

	mTimer.Reset();

	while (msg.message != WM_QUIT) 	{
		// If there are Window messages then process them.
		if (PeekMessage(&msg, 0U, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Otherwise, do animation/game stuff.
		else {
			mTimer.Tick();

			if (!mAppPaused) {
				const float dt = mTimer.DeltaTime();
				CalculateFrameStats();
				Draw(dt);
				Keyboard::gKeyboard->Update();
				Update(dt);				
			}
			else {
				Sleep(100U);
			}
		}
	}

	return (int)msg.wParam;
}

void App::Initialize() noexcept {
	InitMainWindow();
	InitDirect3D();
	InitSystems();

	//tbb::empty_task* parent{ CommandListProcessor::Create(mCmdListProcessor, mCmdQueue) };
	//ASSERT(parent != nullptr);
}

void App::CreateRtvAndDsvDescriptorHeaps() noexcept {
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = sSwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	CHECK_HR(mD3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1U;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0U;
	CHECK_HR(mD3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void App::Update(const float dt) noexcept {
	static const float sCameraOffset{ 10.0f };
	static const float sCameraMultiplier{ 5.0f };

	ASSERT(Keyboard::gKeyboard.get() != nullptr);
	const float offset = sCameraOffset * (Keyboard::gKeyboard->IsKeyDown(DIK_LSHIFT) ? sCameraMultiplier : 1.0f) * dt ;
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

	Camera::gCamera->UpdateViewMatrix();
}

void App::OnMouseMove(const WPARAM btnState, const int32_t x, const int32_t y) noexcept {
	static int32_t lastXY[] = { 0, 0 };

	ASSERT(Camera::gCamera.get() != nullptr);

	if (btnState & MK_LBUTTON) {
		// Make each pixel correspond to a quarter of a degree.+
		const float dx = {DirectX::XMConvertToRadians(0.25f * (float)(x - lastXY[0]))};
		const float dy = {DirectX::XMConvertToRadians(0.25f * (float)(y - lastXY[1]))};

		Camera::gCamera->Pitch(dy);
		Camera::gCamera->RotateY(dx);
	}

	lastXY[0] = x;
	lastXY[1] = y;
}

void App::CreateRtvAndDsv() noexcept {
	ASSERT(mD3dDevice != nullptr);
	ASSERT(mSwapChain != nullptr);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (std::uint32_t i = 0U; i < sSwapChainBufferCount; ++i) {
		CHECK_HR(mSwapChain->GetBuffer(i, IID_PPV_ARGS(mSwapChainBuffer[i].GetAddressOf())));
		mD3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1U, mRtvDescSize);
	}

	// Create the depth/stencil buffer and view.
	D3D12_RESOURCE_DESC depthStencilDesc = {};
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0U;
	depthStencilDesc.Width = mWindowWidth;
	depthStencilDesc.Height = mWindowHeight;
	depthStencilDesc.DepthOrArraySize = 1U;
	depthStencilDesc.MipLevels = 1U;
	depthStencilDesc.Format = mDepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = 1U;
	depthStencilDesc.SampleDesc.Quality = 0U;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear = {};
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0U;
	CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
	CHECK_HR(mD3dDevice->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	mD3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, DepthStencilView());

	// Update the viewport transform to cover the client area.
	mScreenViewport.TopLeftX = 0.0f;
	mScreenViewport.TopLeftY = 0.0f;
	mScreenViewport.Width = static_cast<float>(mWindowWidth);
	mScreenViewport.Height = static_cast<float>(mWindowHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mWindowWidth, mWindowHeight };
}

LRESULT App::MsgProc(HWND hwnd, const int32_t msg, WPARAM wParam, LPARAM lParam) noexcept {
	switch (msg) {
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE) {
			mAppPaused = true;
			mTimer.Stop();
		}
		else {
			mAppPaused = false;
			mTimer.Start();
		}
		return 0;

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mTimer.Stop();
		return 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mTimer.Start();
		return 0;

		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// The WM_MENUCHAR message is sent when a menu is active and the user presses 
		// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE) {
			PostQuitMessage(0);
		}

		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void App::InitSystems() noexcept {
	ASSERT(Camera::gCamera.get() == nullptr);
	Camera::gCamera = std::make_unique<Camera>();
	Camera::gCamera->SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);

	ASSERT(Keyboard::gKeyboard.get() == nullptr);
	LPDIRECTINPUT8 directInput;
	CHECK_HR(DirectInput8Create(mAppInst, DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&directInput, nullptr));
	Keyboard::gKeyboard = std::make_unique<Keyboard>(*directInput, mMainWnd);

	ASSERT(PSOManager::gManager.get() == nullptr);
	PSOManager::gManager = std::make_unique<PSOManager>(*mD3dDevice.Get());

	ASSERT(ResourceManager::gManager.get() == nullptr);
	ResourceManager::gManager = std::make_unique<ResourceManager>(*mD3dDevice.Get());

	ASSERT(RootSignatureManager::gManager.get() == nullptr);
	RootSignatureManager::gManager = std::make_unique<RootSignatureManager>(*mD3dDevice.Get());

	ASSERT(ShaderManager::gManager.get() == nullptr);
	ShaderManager::gManager = std::make_unique<ShaderManager>();
}

void App::InitMainWindow() noexcept {
	WNDCLASS wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = mAppInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd";

	ASSERT(RegisterClass(&wc));

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT r = { 0, 0, mWindowWidth, mWindowHeight };
	AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, false);
	const int32_t width{ r.right - r.left };
	const int32_t height{ r.bottom - r.top };

	const std::uint32_t dwStyle = { WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX };
	mMainWnd = CreateWindow(L"MainWnd", L"App", dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mAppInst, 0);
	ASSERT(mMainWnd);

	ShowWindow(mMainWnd, SW_SHOW);
	UpdateWindow(mMainWnd);
}

void App::InitDirect3D() noexcept {
#if defined(DEBUG) || defined(_DEBUG) 
	// Enable the D3D12 debug layer.
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		CHECK_HR(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())));
		debugController->EnableDebugLayer();
	}
#endif

	// Create device
	CHECK_HR(CreateDXGIFactory1(IID_PPV_ARGS(mDxgiFactory.GetAddressOf())));
	CHECK_HR(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(mD3dDevice.GetAddressOf())));
	
	// Create fence and query descriptors sizes
	CHECK_HR(mD3dDevice->CreateFence(0U, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(mFence.GetAddressOf())));
	mRtvDescSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mSamplerDescSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	
	CreateCommandObjects();
	CreateSwapChain();
	CreateRtvAndDsvDescriptorHeaps();
	CreateRtvAndDsv();
}

void App::CreateCommandObjects() noexcept {
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CHECK_HR(mD3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(mCmdQueue.GetAddressOf())));
	CHECK_HR(mD3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));
	CHECK_HR(mD3dDevice->CreateCommandList(0U, D3D12_COMMAND_LIST_TYPE_DIRECT, mDirectCmdListAlloc.Get(), nullptr, IID_PPV_ARGS(mCmdList.GetAddressOf())));

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	mCmdList->Close();
}

void App::CreateSwapChain() noexcept {
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferDesc.Width = mWindowWidth;
	sd.BufferDesc.Height = mWindowHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60U;
	sd.BufferDesc.RefreshRate.Denominator = 1U;
	sd.BufferDesc.Format = mBackBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count =  1U;
	sd.SampleDesc.Quality = 0U;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = sSwapChainBufferCount;
	sd.OutputWindow = mMainWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain uses queue to perform flush.
	CHECK_HR(mDxgiFactory->CreateSwapChain(mCmdQueue.Get(), &sd, mSwapChain.GetAddressOf()));

	// Resize the swap chain.
	CHECK_HR(mSwapChain->ResizeBuffers(sSwapChainBufferCount, mWindowWidth, mWindowHeight, mBackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
}

void App::FlushCommandQueue() noexcept {
	// Advance the fence value to mark commands up to this fence point.
	++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	CHECK_HR(mCmdQueue->Signal(mFence.Get(), mCurrentFence));
	
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

ID3D12Resource* App::CurrentBackBuffer() const noexcept {
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE App::CurrentBackBufferView() const noexcept {
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mCurrBackBuffer, mRtvDescSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE App::DepthStencilView() const noexcept {
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void App::CalculateFrameStats() noexcept {
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.

	static std::uint32_t frameCnt{ 0U };
	static float timeElapsed{ 0.0f };

	++frameCnt;

	// Compute averages over one second period.
	if ((mTimer.TotalTime() - timeElapsed) > 1.0f) {
		const float mspf{ 1000.0f / frameCnt };
		SetWindowText(mMainWnd, std::to_wstring(mspf).c_str());

		// Reset for next average.
		frameCnt = 0U;
		timeElapsed += 1.0f;
	}
}
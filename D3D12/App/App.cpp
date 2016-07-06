#include "App.h"

#include <Camera/Camera.h>
#include <GlobalData\D3dData.h>
#include <GlobalData/Settings.h>
#include <Input/Keyboard.h>
#include <Input/Mouse.h>
#include <MathUtils/MathHelper.h>
#include <Utils\DebugUtils.h>

namespace {
	const std::uint32_t MAX_NUM_CMD_LISTS{ 3U };
}

App::App(HINSTANCE hInstance)
	: mTaskSchedulerInit()
	, mAppInst(hInstance)
	, mMasterRenderTaskParent(MasterRenderTask::Create(mMasterRenderTask))
{	
	Init();
}

void App::ExecuteInitTasks() noexcept {
	ASSERT(mMasterRenderTask != nullptr);
	mMasterRenderTask->ExecuteInitTasks();
}

int32_t App::Run() noexcept {
	ASSERT(Keyboard::gKeyboard.get() != nullptr);
	ASSERT(Mouse::gMouse.get() != nullptr);

	// Spawn master render task
	ASSERT(mMasterRenderTaskParent != nullptr);
	ASSERT(mMasterRenderTask != nullptr);	
	mMasterRenderTaskParent->spawn(*mMasterRenderTask);

	// Message loop
	MSG msg{0U};
	while (msg.message != WM_QUIT) 	{
		if (PeekMessage(&msg, 0U, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			mTimer.Tick();
			const float dt = mTimer.DeltaTime();
			Keyboard::gKeyboard->Update();
			Mouse::gMouse->Update();
			Update(dt);
		}
	}

	return (std::int32_t)msg.wParam;
}

void App::Init() noexcept {
	InitMainWindow();
	D3dData::InitDirect3D();
	InitSystems();	
}

void App::Update(const float dt) noexcept {
	static std::int32_t lastXY[]{ 0UL, 0UL };
	static const float sCameraOffset{ 10.0f };
	static const float sCameraMultiplier{ 5.0f };

	ASSERT(Keyboard::gKeyboard.get() != nullptr);
	ASSERT(Mouse::gMouse.get() != nullptr);

	// Check if we need to terminate application
	if (Keyboard::gKeyboard->IsKeyDown(DIK_ESCAPE)) {
		Finalize();		
	}

	// Update camera based on keyboard 
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

	// Update camera based on mouse
	const std::int32_t x{ Mouse::gMouse->X() };
	const std::int32_t y{ Mouse::gMouse->Y() };
	if (Mouse::gMouse->IsButtonDown(Mouse::MouseButtonsLeft)) {
		// Make each pixel correspond to a quarter of a degree.
		const float dx{ DirectX::XMConvertToRadians(0.25f * (float)(x - lastXY[0])) };
		const float dy{DirectX::XMConvertToRadians(0.25f * (float)(y - lastXY[1])) };

		Camera::gCamera->Pitch(dy);
		Camera::gCamera->RotateY(dx);
	}

	lastXY[0] = x;
	lastXY[1] = y;
}

void App::Finalize() noexcept {
	// Terminate master render task and wait for its finalization
	mMasterRenderTask->Terminate();
	mMasterRenderTaskParent->wait_for_all();
	mTaskSchedulerInit.terminate();
	PostQuitMessage(0);
}

void App::InitSystems() noexcept {
	ASSERT(Camera::gCamera.get() == nullptr);
	Camera::gCamera = std::make_unique<Camera>();
	Camera::gCamera->SetLens(0.25f * MathHelper::Pi, Settings::AspectRatio(), 1.0f, 1000.0f);
	Camera::gCamera->UpdateViewMatrix();

	ASSERT(Keyboard::gKeyboard.get() == nullptr);
	LPDIRECTINPUT8 directInput;
	CHECK_HR(DirectInput8Create(mAppInst, DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&directInput, nullptr));
	Keyboard::gKeyboard = std::make_unique<Keyboard>(*directInput, mHwnd);

	ASSERT(Mouse::gMouse.get() == nullptr);
	Mouse::gMouse = std::make_unique<Mouse>(*directInput, mHwnd);

	mMasterRenderTask->Init(mHwnd);
}

void App::InitMainWindow() noexcept {
	WNDCLASS wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = DefWindowProc;
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
	RECT r = { 0, 0, Settings::sWindowWidth, Settings::sWindowHeight };
	AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, false);
	const int32_t width{ r.right - r.left };
	const int32_t height{ r.bottom - r.top };

	const std::uint32_t dwStyle = Settings::sFullscreen ?  WS_POPUP : WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
	mHwnd = CreateWindowEx(WS_EX_APPWINDOW, L"MainWnd", L"App", dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, mAppInst, 0);
	ASSERT(mHwnd);

	ShowWindow(mHwnd, SW_SHOW);
	UpdateWindow(mHwnd);
}
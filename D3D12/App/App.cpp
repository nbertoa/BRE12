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
	, mMasterRenderTaskParent(MasterRenderTask::Create(mMasterRenderTask))
{	
	Init(hInstance);
}

void App::InitCmdBuilders() noexcept {
	ASSERT(mMasterRenderTask != nullptr);
	mMasterRenderTask->InitCmdBuilders();
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

void App::Init(const HINSTANCE hInstance) noexcept {
	D3dData::InitDirect3D(hInstance);
	InitSystems(D3dData::mHwnd, hInstance);
}

void App::InitSystems(const HWND hwnd, const HINSTANCE hInstance) noexcept {
	ASSERT(Camera::gCamera.get() == nullptr);
	Camera::gCamera = std::make_unique<Camera>();
	Camera::gCamera->SetLens(0.25f * MathHelper::Pi, Settings::AspectRatio(), 1.0f, 1000.0f);
	Camera::gCamera->UpdateViewMatrix();

	ASSERT(Keyboard::gKeyboard.get() == nullptr);
	LPDIRECTINPUT8 directInput;
	CHECK_HR(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&directInput, nullptr));
	Keyboard::gKeyboard = std::make_unique<Keyboard>(*directInput, hwnd);

	ASSERT(Mouse::gMouse.get() == nullptr);
	Mouse::gMouse = std::make_unique<Mouse>(*directInput, hwnd);

	mMasterRenderTask->Init(hwnd);
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
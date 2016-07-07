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

using namespace DirectX;

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
			Update();
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
	Camera::gCamera->SetLens(Settings::sFieldOfView, Settings::AspectRatio(), Settings::sNearPlaneZ, Settings::sFarPlaneZ);

	ASSERT(Keyboard::gKeyboard.get() == nullptr);
	LPDIRECTINPUT8 directInput;
	CHECK_HR(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&directInput, nullptr));
	Keyboard::gKeyboard = std::make_unique<Keyboard>(*directInput, hwnd);

	ASSERT(Mouse::gMouse.get() == nullptr);
	Mouse::gMouse = std::make_unique<Mouse>(*directInput, hwnd);

	mMasterRenderTask->Init(hwnd);
}

void App::Update() noexcept {
	ASSERT(Keyboard::gKeyboard.get() != nullptr);
	ASSERT(Mouse::gMouse.get() != nullptr);

	Keyboard::gKeyboard->Update();
	Mouse::gMouse->Update();
	if (Keyboard::gKeyboard->IsKeyDown(DIK_ESCAPE)) {
		Finalize();		
	}
}

void App::Finalize() noexcept {
	// Terminate master render task and wait for its finalization
	mMasterRenderTask->Terminate();
	mMasterRenderTaskParent->wait_for_all();
	mTaskSchedulerInit.terminate();
	PostQuitMessage(0);
}
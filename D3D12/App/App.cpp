#include "App.h"

#include <Camera/Camera.h>
#include <CommandManager/CommandManager.h>
#include <GlobalData\D3dData.h>
#include <GlobalData/Settings.h>
#include <Input/Keyboard.h>
#include <Input/Mouse.h>
#include <MathUtils/MathHelper.h>
#include <PSOManager\PSOManager.h>
#include <ResourceManager\ResourceManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <ShaderManager\ShaderManager.h>
#include <Utils\DebugUtils.h>

namespace {
	const std::uint32_t MAX_NUM_CMD_LISTS{ 3U };

	void InitSystems(const HWND hwnd, const HINSTANCE hInstance) noexcept {
		ASSERT(Camera::gCamera.get() == nullptr);
		Camera::gCamera = std::make_unique<Camera>();
		Camera::gCamera->SetLens(Settings::sFieldOfView, Settings::AspectRatio(), Settings::sNearPlaneZ, Settings::sFarPlaneZ);

		ASSERT(Keyboard::gKeyboard.get() == nullptr);
		LPDIRECTINPUT8 directInput;
		CHECK_HR(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&directInput, nullptr));
		Keyboard::gKeyboard = std::make_unique<Keyboard>(*directInput, hwnd);

		ASSERT(Mouse::gMouse.get() == nullptr);
		Mouse::gMouse = std::make_unique<Mouse>(*directInput, hwnd);

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
}

using namespace DirectX;

App::App(HINSTANCE hInstance, Scene* scene)
	: mTaskSchedulerInit()
{	
	ASSERT(scene != nullptr);

	D3dData::InitDirect3D(hInstance);
	const HWND hwnd{ D3dData::mHwnd };
	InitSystems(hwnd, hInstance);
	InitMasterRenderTask(hwnd, scene);
}

void App::InitMasterRenderTask(const HWND hwnd, Scene* scene) noexcept {
	ASSERT(scene != nullptr);
	ASSERT(mMasterRender == nullptr);
	mMasterRender = MasterRender::Create(hwnd, scene);
}

std::int32_t App::Run() noexcept {
	ASSERT(Keyboard::gKeyboard.get() != nullptr);
	ASSERT(Mouse::gMouse.get() != nullptr);

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
	mMasterRender->Terminate();
	mTaskSchedulerInit.terminate();
	PostQuitMessage(0);
}
#include "App.h"

#include <CommandManager/CommandManager.h>
#include <DescriptorManager/DescriptorManager.h>
#include <DirectXManager\DirectXManager.h>
#include <Input/Keyboard.h>
#include <Input/Mouse.h>
#include <MasterRender/MasterRender.h>
#include <Material/Material.h>
#include <ModelManager\ModelManager.h>
#include <PSOManager\PSOManager.h>
#include <ResourceManager\ResourceManager.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <ShaderManager\ShaderManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils\DebugUtils.h>

using namespace DirectX;

namespace {
	void InitSystems(const HWND windowHandle, 
					 const HINSTANCE moduleInstanceHandle) noexcept 
	{
		LPDIRECTINPUT8 directInput;
		CHECK_HR(DirectInput8Create(moduleInstanceHandle, 
									DIRECTINPUT_VERSION, 
									IID_IDirectInput8, 
									reinterpret_cast<LPVOID*>(&directInput), 
									nullptr));
		Keyboard::Create(*directInput, windowHandle);
		Mouse::Create(*directInput, windowHandle);

		CommandManager::Create();
		DescriptorManager::Create();
		ModelManager::Create();
		Materials::InitMaterials();
		PSOManager::Create();
		ResourceManager::Create();
		ResourceStateManager::Create();
		RootSignatureManager::Create();
		ShaderManager::Create();

		//ShowCursor(false);
	}

	void UpdateKeyboardAndMouse() noexcept {
		Keyboard::Get().Update();
		Mouse::Get().Update();
		if (Keyboard::Get().IsKeyDown(DIK_ESCAPE)) {
			PostQuitMessage(0);
		}
	}

	// Runs program until Escape key is pressed.
	std::int32_t RunMessageLoop() noexcept {
		MSG message{ nullptr };
		while (message.message != WM_QUIT) {
			if (PeekMessage(&message, nullptr, 0U, 0U, PM_REMOVE)) {
				TranslateMessage(&message);
				DispatchMessage(&message);
			}
			else {
				UpdateKeyboardAndMouse();
			}
		}

		return static_cast<std::int32_t>(message.wParam);
	}
}

using namespace DirectX;

App::App(HINSTANCE hInstance, Scene* scene)
	: mTaskSchedulerInit()
{	
	ASSERT(scene != nullptr);
	DirectXManager::InitDirect3D(hInstance);
	InitSystems(DirectXManager::WindowHandle(), hInstance);
	mMasterRender = MasterRender::Create(DirectXManager::WindowHandle(), scene);

	RunMessageLoop();
}

App::~App() {	
	ASSERT(mMasterRender != nullptr);
	mMasterRender->Terminate();
	mTaskSchedulerInit.terminate();
}
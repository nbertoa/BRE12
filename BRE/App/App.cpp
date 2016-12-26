#include "App.h"

#include <CommandManager/CommandManager.h>
#include <DescriptorManager/DescriptorManager.h>
#include <GlobalData\D3dData.h>
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
	void InitSystems(const HWND hwnd, const HINSTANCE hInstance) noexcept {
		LPDIRECTINPUT8 directInput;
		CHECK_HR(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, reinterpret_cast<LPVOID*>(&directInput), nullptr));
		Keyboard::Create(*directInput, hwnd);
		Mouse::Create(*directInput, hwnd);

		ID3D12Device& device{ D3dData::Device() };
		CommandManager::Create(device);
		DescriptorManager::Create(device);
		ModelManager::Create();
		Materials::InitMaterials();
		PSOManager::Create(device);
		ResourceManager::Create(device);
		ResourceStateManager::Create();
		RootSignatureManager::Create(device);
		ShaderManager::Create();

		//ShowCursor(false);
	}

	void InitMasterRenderTask(const HWND hwnd, Scene* scene, MasterRender* &masterRender) noexcept {
		ASSERT(scene != nullptr);
		ASSERT(masterRender == nullptr);
		masterRender = MasterRender::Create(hwnd,  scene);
	}

	void Update() noexcept {
		Keyboard::Get().Update();
		Mouse::Get().Update();
		if (Keyboard::Get().IsKeyDown(DIK_ESCAPE)) {
			PostQuitMessage(0);
		}
	}

	// Runs program until Escape key is pressed.
	std::int32_t RunMessageLoop() noexcept {
		// Message loop
		MSG msg{ nullptr };
		while (msg.message != WM_QUIT) {
			if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else {
				Update();
			}
		}

		return static_cast<std::int32_t>(msg.wParam);
	}
}

using namespace DirectX;

App::App(HINSTANCE hInstance, Scene* scene)
	: mTaskSchedulerInit()
{	
	ASSERT(scene != nullptr);
	D3dData::InitDirect3D(hInstance);
	InitSystems(D3dData::Hwnd(), hInstance);
	InitMasterRenderTask(D3dData::Hwnd(), scene, mMasterRender);

	RunMessageLoop();
}

App::~App() {	ASSERT(mMasterRender != nullptr);
	mMasterRender->Terminate();
	mTaskSchedulerInit.terminate();
}
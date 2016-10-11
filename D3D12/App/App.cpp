#include "App.h"

#include <CommandManager/CommandManager.h>
#include <GeometryPass/MaterialFactory.h>
#include <GlobalData\D3dData.h>
#include <GlobalData/Settings.h>
#include <Input/Keyboard.h>
#include <Input/Mouse.h>
#include <MasterRender/MasterRender.h>
#include <ModelManager\ModelManager.h>
#include <PSOManager\PSOManager.h>
#include <ResourceManager\ResourceManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <ShaderManager\ShaderManager.h>
#include <Utils\DebugUtils.h>

namespace {
	void InitSystems(const HWND hwnd, const HINSTANCE hInstance) noexcept {
		LPDIRECTINPUT8 directInput;
		CHECK_HR(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&directInput, nullptr));
		Keyboard::Create(*directInput, hwnd);
		Mouse::Create(*directInput, hwnd);

		ID3D12Device& device{ D3dData::Device() };
		CommandManager::Create(device);
		ModelManager::Create();
		MaterialFactory::InitMaterials();
		PSOManager::Create(device);
		ResourceManager::Create(device);
		RootSignatureManager::Create(device);
		ShaderManager::Create();
	}

	void InitMasterRenderTask(const HWND hwnd, ID3D12Device& device, Scene* scene, MasterRender* &masterRender) noexcept {
		ASSERT(scene != nullptr);
		ASSERT(masterRender == nullptr);
		masterRender = MasterRender::Create(hwnd, device, scene);
	}

	void Update() noexcept {
		Keyboard::Get().Update();
		Mouse::Get().Update();
		if (Keyboard::Get().IsKeyDown(DIK_ESCAPE)) {
			PostQuitMessage(0);
		}
	}
}

using namespace DirectX;

App::App(HINSTANCE hInstance, Scene* scene)
	: mTaskSchedulerInit()
{	
	ASSERT(scene != nullptr);
	D3dData::InitDirect3D(hInstance);
	InitSystems(D3dData::Hwnd(), hInstance);
	InitMasterRenderTask(D3dData::Hwnd(), D3dData::Device(), scene, mMasterRender);
}

App::~App() {
	ASSERT(mMasterRender != nullptr);
	mMasterRender->Terminate();
	mTaskSchedulerInit.terminate();
}

std::int32_t App::Run() noexcept {
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
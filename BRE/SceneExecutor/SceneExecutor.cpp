#include "SceneExecutor.h"

#include <memory>

#include <CommandManager/CommandAllocatorManager.h>
#include <CommandManager/CommandListManager.h>
#include <CommandManager/CommandQueueManager.h>
#include <DescriptorManager/CbvSrvUavDescriptorManager.h>
#include <DescriptorManager/DepthStencilDescriptorManager.h>
#include <DescriptorManager/RenderTargetDescriptorManager.h>
#include <DirectXManager\DirectXManager.h>
#include <Input/Keyboard.h>
#include <Input/Mouse.h>
#include <MasterRender/MasterRender.h>
#include <MaterialManager/MaterialManager.h>
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
	std::unique_ptr<SceneExecutor> gSceneExecutor{ nullptr };

	void CreateSystems(const HINSTANCE moduleInstanceHandle) noexcept 
	{
		const HWND windowHandle = DirectXManager::GetWindowHandle();

		LPDIRECTINPUT8 directInput;
		CHECK_HR(DirectInput8Create(moduleInstanceHandle, 
									DIRECTINPUT_VERSION, 
									IID_IDirectInput8, 
									reinterpret_cast<LPVOID*>(&directInput), 
									nullptr));
		Keyboard::Create(*directInput, windowHandle);
		Mouse::Create(*directInput, windowHandle);

		CommandAllocatorManager::Create();
		CommandListManager::Create();
		CommandQueueManager::Create();
		CbvSrvUavDescriptorManager::Create();
		DepthStencilDescriptorManager::Create();
		RenderTargetDescriptorManager::Create();
		ModelManager::Create();
		MaterialManager::Create();
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

SceneExecutor& SceneExecutor::Create(HINSTANCE moduleInstanceHandle, Scene* scene) noexcept {
	ASSERT(gSceneExecutor == nullptr);
	gSceneExecutor.reset(new SceneExecutor(moduleInstanceHandle, scene));
	return *gSceneExecutor.get();
}
SceneExecutor& SceneExecutor::Get() noexcept {
	ASSERT(gSceneExecutor != nullptr);
	return *gSceneExecutor.get();
}

void SceneExecutor::Destroy() noexcept {
	ASSERT(gSceneExecutor != nullptr);
	gSceneExecutor.reset();
}

SceneExecutor::~SceneExecutor() {
	ASSERT(mMasterRender != nullptr);
	mMasterRender->Terminate();
	mTaskSchedulerInit.terminate();
}

void SceneExecutor::Execute() noexcept {
	RunMessageLoop();
}

SceneExecutor::SceneExecutor(HINSTANCE moduleInstanceHandle, Scene* scene)
	: mTaskSchedulerInit()
	, mScene(scene)
{
	ASSERT(scene != nullptr);
	DirectXManager::Init(moduleInstanceHandle);
	CreateSystems(moduleInstanceHandle);
	mMasterRender = MasterRender::Create(DirectXManager::GetWindowHandle(), *mScene.get());	
}
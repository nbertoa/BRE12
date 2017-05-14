#include "SceneExecutor.h"

#include <memory>

#include <CommandListExecutor\CommandListExecutor.h>
#include <CommandManager\CommandAllocatorManager.h>
#include <CommandManager/CommandListManager.h>
#include <CommandManager\CommandQueueManager.h>
#include <CommandManager\FenceManager.h>
#include <DescriptorManager/CbvSrvUavDescriptorManager.h>
#include <DescriptorManager/DepthStencilDescriptorManager.h>
#include <DescriptorManager/RenderTargetDescriptorManager.h>
#include <DirectXManager\DirectXManager.h>
#include <Input/Keyboard.h>
#include <Input/Mouse.h>
#include <PSOManager\PSOManager.h>
#include <RenderManager/RenderManager.h>
#include <ResourceManager\ResourceManager.h>
#include <ResourceManager\UploadBufferManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <SceneLoader\SceneLoader.h>
#include <ShaderManager\ShaderManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils\DebugUtils.h>

using namespace DirectX;

namespace BRE {
namespace {
const std::uint32_t MAX_NUM_CMD_LISTS{ 3U };

void InitSystems(const HINSTANCE moduleInstanceHandle) noexcept
{
    const HWND windowHandle = DirectXManager::GetWindowHandle();

    LPDIRECTINPUT8 directInput;
    BRE_CHECK_HR(DirectInput8Create(moduleInstanceHandle,
                                    DIRECTINPUT_VERSION,
                                    IID_IDirectInput8,
                                    reinterpret_cast<LPVOID*>(&directInput),
                                    nullptr));
    Keyboard::Create(*directInput, windowHandle);
    Mouse::Create(*directInput, windowHandle);

    CbvSrvUavDescriptorManager::Init();
    DepthStencilDescriptorManager::Init();
    RenderTargetDescriptorManager::Init();

    CommandListExecutor::Create(MAX_NUM_CMD_LISTS);

    //ShowCursor(false);
}

void FinalizeSystems() noexcept
{
    CommandAllocatorManager::Clear();
    CommandListManager::Clear();
    CommandQueueManager::Clear();
    FenceManager::Clear();
    PSOManager::EraseAll();
    ResourceManager::EraseAll();
    RootSignatureManager::EraseAll();
    ShaderManager::EraseAll();
    UploadBufferManager::EraseAll();
}

void UpdateKeyboardAndMouse() noexcept
{
    Keyboard::Get().UpdateKeysState();
    Mouse::Get().UpdateMouseState();
    if (Keyboard::Get().IsKeyDown(DIK_ESCAPE)) {
        PostQuitMessage(0);
    }
}

// Runs program until Escape key is pressed.
std::int32_t RunMessageLoop() noexcept
{
    MSG message{ nullptr };
    while (message.message != WM_QUIT) {
        if (PeekMessage(&message, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessage(&message);
        } else {
            UpdateKeyboardAndMouse();
        }
    }

    return static_cast<std::int32_t>(message.wParam);
}
}

using namespace DirectX;

SceneExecutor::~SceneExecutor()
{
    BRE_ASSERT(mRenderManager != nullptr);
    mRenderManager->Terminate();
    mTaskSchedulerInit.terminate();

    FinalizeSystems();

    delete mScene;
}

void
SceneExecutor::Execute() noexcept
{
    RunMessageLoop();
}

SceneExecutor::SceneExecutor(HINSTANCE moduleInstanceHandle,
                             const char* sceneFilePath)
    : mTaskSchedulerInit()
{
    BRE_ASSERT(sceneFilePath != nullptr);

    DirectXManager::InitWindowAndDevice(moduleInstanceHandle);
    InitSystems(moduleInstanceHandle);

    SceneLoader sceneLoader;
    mScene = sceneLoader.LoadScene(sceneFilePath);
    BRE_ASSERT(mScene != nullptr);

    mRenderManager = &RenderManager::Create(*mScene);
}
}


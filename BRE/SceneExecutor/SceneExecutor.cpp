#include "SceneExecutor.h"

#include <CommandListExecutor\CommandListExecutor.h>
#include <Input/Keyboard.h>
#include <Input/Mouse.h>
#include <RenderManager/RenderManager.h>
#include <Scene/Scene.h>
#include <SceneLoader\SceneLoader.h>
#include <Utils\DebugUtils.h>

using namespace DirectX;

namespace BRE {
namespace {
const std::uint32_t MAX_NUM_CMD_LISTS{ 3U };

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

    delete mScene;
}

void
SceneExecutor::Execute() noexcept
{
    RunMessageLoop();
}

SceneExecutor::SceneExecutor(const char* sceneFilePath)
{
    BRE_ASSERT(sceneFilePath != nullptr);

    CommandListExecutor::Create(MAX_NUM_CMD_LISTS);

    SceneLoader sceneLoader;
    mScene = sceneLoader.LoadScene(sceneFilePath);
    BRE_ASSERT(mScene != nullptr);

    mRenderManager = &RenderManager::Create(*mScene);
}
}
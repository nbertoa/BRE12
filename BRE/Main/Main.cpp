#include <tbb/task_scheduler_init.h>
#include <windows.h>

#pragma warning( push )
#pragma warning( disable : 4127)
#include <yaml-cpp/yaml.h>
#pragma warning( pop ) 

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
#include <ResourceManager\ResourceManager.h>
#include <ResourceManager\UploadBufferManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <SceneExecutor/SceneExecutor.h>
#include <SceneLoader\SceneLoader.h>
#include <SceneLoader\SettingsLoader.h>
#include <ShaderManager\ShaderManager.h>
#include <Utils\DebugUtils.h>

#if defined(DEBUG) || defined(_DEBUG)                                                                                                                                                            
#define _CRTDBG_MAP_ALLOC          
#include <cstdlib>             
#include <crtdbg.h>               
#endif 

namespace BRE {
namespace {
const std::uint32_t RENDER_TARGET_DESCRIPTOR_HEAP_SIZE = 10U;
const std::uint32_t CBV_SRV_UAV_DESCRIPTOR_HEAP_SIZE = 3000U;

///
/// @brief Initializes all the systems
/// @param moduleInstanceHandle Module instance handle to create the window
///
void InitSystems(const HINSTANCE moduleInstanceHandle,
                 const char* sceneFilePath) noexcept
{
    BRE_ASSERT(sceneFilePath != nullptr);

    // Load settings
    const YAML::Node rootNode = YAML::LoadFile(sceneFilePath);
    const std::wstring errorMsg = 
        L"Failed to open yaml file: " + StringUtils::AnsiToWideString(sceneFilePath);
    BRE_CHECK_MSG(rootNode.IsDefined(), errorMsg.c_str());
    SettingsLoader settingsLoader;
    settingsLoader.LoadSettings(rootNode);

    DirectXManager::InitWindowAndDevice(moduleInstanceHandle);
    const HWND windowHandle = DirectXManager::GetWindowHandle();

    LPDIRECTINPUT8 directInput;
    BRE_CHECK_HR(DirectInput8Create(moduleInstanceHandle,
                                    DIRECTINPUT_VERSION,
                                    IID_IDirectInput8,
                                    reinterpret_cast<LPVOID*>(&directInput),
                                    nullptr));
    Keyboard::Create(*directInput, windowHandle);
    Mouse::Create(*directInput, windowHandle);

    CbvSrvUavDescriptorManager::Init(CBV_SRV_UAV_DESCRIPTOR_HEAP_SIZE);
    DepthStencilDescriptorManager::Init();
    RenderTargetDescriptorManager::Init(RENDER_TARGET_DESCRIPTOR_HEAP_SIZE);

    //ShowCursor(false);
}

///
/// @brief Finalize all the systems
///
void FinalizeSystems() noexcept
{
    CommandAllocatorManager::Clear();
    CommandListManager::Clear();
    CommandQueueManager::Clear();
    FenceManager::Clear();
    PSOManager::Clear();
    ResourceManager::Clear();
    RootSignatureManager::Clear();
    ShaderManager::Clear();
    UploadBufferManager::Clear();
}
}
}

int
WINAPI WinMain(_In_ HINSTANCE moduleInstanceHandle,
               _In_opt_ HINSTANCE /*previousModuleInstanceHandle*/,
               _In_ LPSTR /*commandLine*/,
               _In_ int /*showCommand*/)
{
    const char* sceneFilePath = "resources/scenes/showcase2.yml";

    tbb::task_scheduler_init taskSchedulerInit;

    BRE::InitSystems(moduleInstanceHandle,
                     sceneFilePath);

    {
        BRE::SceneExecutor sceneExecutor(sceneFilePath);
        sceneExecutor.Execute();
    }

    taskSchedulerInit.terminate();
    BRE::FinalizeSystems();

    return 0;
}
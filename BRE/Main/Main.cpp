#include <windows.h>

#include <tbb/task_scheduler_init.h>
#include <windows.h>

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
#include <ShaderManager\ShaderManager.h>

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
void InitSystems(const HINSTANCE moduleInstanceHandle) noexcept
{
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
    tbb::task_scheduler_init taskSchedulerInit;

    BRE::InitSystems(moduleInstanceHandle);

    {
        BRE::SceneExecutor sceneExecutor("resources/scenes/non_metal_smoothness.yml");
        sceneExecutor.Execute();
    }

    taskSchedulerInit.terminate();
    BRE::FinalizeSystems();

    return 0;
}
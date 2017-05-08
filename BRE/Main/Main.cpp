#include <windows.h>

#include <SceneExecutor/SceneExecutor.h>

int
WINAPI WinMain(_In_ HINSTANCE moduleInstanceHandle,
               _In_opt_ HINSTANCE /*previousModuleInstanceHandle*/,
               _In_ LPSTR /*commandLine*/,
               _In_ int /*showCommand*/)
{
    SceneExecutor sceneExecutor(moduleInstanceHandle, "resources/scenes/test.yml");
    sceneExecutor.Execute();

    return 0;
}
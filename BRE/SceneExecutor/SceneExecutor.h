#pragma once

#include <memory>
#include <tbb/task_scheduler_init.h>
#include <windows.h>

#include <Scene\Scene.h>

#if defined(DEBUG) || defined(_DEBUG)                                                                                                                                                            
#define _CRTDBG_MAP_ALLOC          
#include <cstdlib>             
#include <crtdbg.h>               
#endif 

class RenderManager;

// To execute a scene.
class SceneExecutor {
public:
    explicit SceneExecutor(HINSTANCE moduleInstanceHandle,
                           const char* sceneFilePath);
    ~SceneExecutor();
    SceneExecutor(const SceneExecutor&) = delete;
    const SceneExecutor& operator=(const SceneExecutor&) = delete;
    SceneExecutor(SceneExecutor&&) = delete;
    SceneExecutor& operator=(SceneExecutor&&) = delete;

    void Execute() noexcept;

private:
    // Needed by Intel TBB
    tbb::task_scheduler_init mTaskSchedulerInit;

    Scene* mScene;

    RenderManager* mRenderManager{ nullptr };
};
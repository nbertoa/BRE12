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

class MasterRender;

// To execute a scene.
class SceneExecutor {
public:
	// Preconditions:
	// - Create() must be called once
	// - SceneExecutor has ownership of "scene" memory.
	static SceneExecutor& Create(HINSTANCE moduleInstanceHandle, Scene* scene) noexcept;

	// Preconditions:
	// - Create() must be called before this method
	static SceneExecutor& Get() noexcept;

	// Preconditions:
	// - Create() must be called before this method
	static void Destroy() noexcept;
	
	~SceneExecutor();
	SceneExecutor(const SceneExecutor&) = delete;
	const SceneExecutor& operator=(const SceneExecutor&) = delete;
	SceneExecutor(SceneExecutor&&) = delete;
	SceneExecutor& operator=(SceneExecutor&&) = delete;

	void Execute() noexcept;
	
private:	
	explicit SceneExecutor(HINSTANCE moduleInstanceHandle, Scene* scene);

	// Needed by Intel TBB
	tbb::task_scheduler_init mTaskSchedulerInit;

	MasterRender* mMasterRender{ nullptr };
	std::unique_ptr<Scene> mScene;
};
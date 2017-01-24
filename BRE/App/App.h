#pragma once

#include <tbb/task_scheduler_init.h>
#include <windows.h>

#if defined(DEBUG) || defined(_DEBUG)                                                                                                                                                            
#define _CRTDBG_MAP_ALLOC          
#include <cstdlib>             
#include <crtdbg.h>               
#endif 

class MasterRender;
class Scene;

// To initialize Direct3D systems, mouse, keyboard, camera, MasterRender, etc.
class App {
public:
	explicit App(HINSTANCE moduleInstanceHandle, Scene* scene);
	~App();
	App(const App&) = delete;
	const App& operator=(const App&) = delete;
	App(App&&) = delete;
	App& operator=(App&&) = delete;
	
private:	
	void InitMasterRenderTask(Scene* scene) noexcept;

	// Needed by Intel TBB
	tbb::task_scheduler_init mTaskSchedulerInit;

	MasterRender* mMasterRender{ nullptr };
};
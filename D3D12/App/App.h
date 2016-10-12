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

// Its responsibility is to initialize Direct3D systems, mouse, keyboard, camera, MasterRender, etc
class App {
public:
	explicit App(HINSTANCE hInstance, Scene* scene);
	~App();
	App(const App& rhs) = delete;
	App& operator=(const App& rhs) = delete;		

	// Runs program until Escape key is pressed.
	// TODO
	static std::int32_t Run() noexcept;

private:	
	// Needed by Intel TBB
	tbb::task_scheduler_init mTaskSchedulerInit;

	// Master render 
	MasterRender* mMasterRender{ nullptr };
};
#pragma once

#include <tbb/task_scheduler_init.h>
#include <vector>
#include <windows.h>

#include <App/MasterRender.h>

#if defined(DEBUG) || defined(_DEBUG)                                                                                                                                                            
#define _CRTDBG_MAP_ALLOC          
#include <cstdlib>             
#include <crtdbg.h>               
#endif 

// Its responsibility is to initialize Direct3D systems, mouse, keyboard, camera, MasterRender, etc
class App {
public:
	explicit App(HINSTANCE hInstance, Scene* scene);
	App(const App& rhs) = delete;
	App& operator=(const App& rhs) = delete;		

	// Runs program until Esc key is pressed.
	std::int32_t Run() noexcept;

protected:	
	void InitMasterRenderTask(const HWND hwnd, Scene* scene) noexcept;

	void Update() noexcept;
	void Finalize() noexcept;
	
	// Needed by Intel TBB
	tbb::task_scheduler_init mTaskSchedulerInit;

	// Master render 
	MasterRender* mMasterRender{ nullptr };
};
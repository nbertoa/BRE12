#pragma once

#include <tbb/task_scheduler_init.h>
#include <vector>
#include <windows.h>

#include <App/MasterRenderTask.h>
#include <RenderTask/CmdBuilderTask.h>
#include <RenderTask/InitTask.h>
#include <Timer\Timer.h>

#if defined(DEBUG) || defined(_DEBUG)                                                                                                                                                            
#define _CRTDBG_MAP_ALLOC          
#include <cstdlib>             
#include <crtdbg.h>               
#endif 

class App {
public:
	explicit App(HINSTANCE hInstance);
	App(const App& rhs) = delete;
	App& operator=(const App& rhs) = delete;
	
	// Execute these methods in declaration order.		
	void ExecuteInitTasks() noexcept;
	std::int32_t Run() noexcept;
	
	__forceinline std::vector<std::unique_ptr<InitTask>>& GetInitTasks() noexcept { ASSERT(mMasterRenderTask != nullptr); return mMasterRenderTask->GetInitTasks(); }
	__forceinline std::vector<std::unique_ptr<CmdBuilderTask>>& GetCmdBuilderTasks() noexcept { ASSERT(mMasterRenderTask != nullptr); return mMasterRenderTask->GetCmdBuilderTasks(); }

protected:	
	void Init() noexcept;
	void Update(const float dt) noexcept;
	void Finalize() noexcept;
	
	void InitSystems() noexcept;
	void InitMainWindow() noexcept;
		
	// This app spawns a master render thread.
	tbb::task_scheduler_init mTaskSchedulerInit;
	MasterRenderTask* mMasterRenderTask{ nullptr };
	tbb::empty_task* mMasterRenderTaskParent{ nullptr };

	HINSTANCE mAppInst = nullptr; // application instance handle
	HWND mHwnd = nullptr; // main window handle
	
	Timer mTimer;
};

// Inherit from this class and implement the method that must:
// - Initialize InitTasks
// - Initialize CmdBuilderTasks
// - Execute InitTasks
class TasksInitializer {
public:
	TasksInitializer() = default;
	virtual void InitTasks(App& app) noexcept = 0;
};
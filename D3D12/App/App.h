#pragma once

#include <cstdint>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <tbb/task_scheduler_init.h>
#include <windows.h>
#include <wrl.h>

#include <App/CommandListProcessor.h>
#include <App/MasterRenderTask.h>
#include <GlobalData/Settings.h>
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
	virtual ~App();

	static App* GetApp() noexcept { return mApp; }	
	
	// Execute these methods in declaration order.
	void Initialize() noexcept;
	void InitializeTasks() noexcept;	
	std::int32_t Run() noexcept;

	LRESULT MsgProc(HWND hwnd, const std::int32_t msg, WPARAM wParam, LPARAM lParam) noexcept;

	__forceinline std::vector<std::unique_ptr<InitTask>>& InitTasks() noexcept { ASSERT(mMasterRenderTask != nullptr); return mMasterRenderTask->InitTasks(); }
	__forceinline std::vector<std::unique_ptr<CmdBuilderTask>>& CmdBuilderTasks() noexcept { ASSERT(mMasterRenderTask != nullptr); return mMasterRenderTask->CmdBuilderTasks(); }

protected:
	static App* mApp;	

	void CreateRtvAndDsvDescriptorHeaps() noexcept;
	void CreateRtvAndDsv() noexcept;
	virtual void Update(const float dt) noexcept;
	void Draw(const float dt) noexcept;
	
	void InitSystems() noexcept;
	void InitMainWindow() noexcept;
	void InitDirect3D() noexcept;	
	void CreateCommandObjects() noexcept;

	void FlushCommandQueue() noexcept;
	void SignalFenceAndPresent() noexcept;
		
	ID3D12Resource* CurrentBackBuffer() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const noexcept;
		
	// Class that is executed in an async thread and get/execute command lists.
	tbb::task_scheduler_init mTaskSchedulerInit;
	CommandListProcessor* mCmdListProcessor{ nullptr };
	MasterRenderTask* mMasterRenderTask{ nullptr };
	tbb::empty_task* mMasterRenderTaskParent{ nullptr };

	HINSTANCE mAppInst = nullptr; // application instance handle
	HWND mHwnd = nullptr; // main window handle
	bool mAppPaused = false;  // is the application paused?

	Timer mTimer;

	std::vector<std::unique_ptr<InitTask>> mInitTasks;
	std::vector<std::unique_ptr<CmdBuilderTask>> mCmdBuilderTasks;

	ID3D12Fence* mFence{ nullptr };
	std::uint64_t mFenceByFrameIndex[Settings::sSwapChainBufferCount]{ 0UL };
	std::uint64_t mCurrentFence{ 0UL };

	ID3D12CommandQueue* mCmdQueue{ nullptr };

	// We have 2 commands lists (for frame begin and frame end), and 2
	// commands allocator per list
	ID3D12CommandAllocator* mCmdAllocFrameBegin[Settings::sSwapChainBufferCount]{ nullptr };
	ID3D12CommandAllocator* mCmdAllocFrameEnd[Settings::sSwapChainBufferCount]{ nullptr };
	ID3D12GraphicsCommandList* mCmdListFrameBegin{ nullptr };
	ID3D12GraphicsCommandList* mCmdListFrameEnd{ nullptr };
	
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[Settings::sSwapChainBufferCount];
	ID3D12Resource* mDepthStencilBuffer{ nullptr };

	ID3D12DescriptorHeap* mRtvHeap{ nullptr };
	ID3D12DescriptorHeap* mDsvHeap{ nullptr };	
};
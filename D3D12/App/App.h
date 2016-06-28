#pragma once

#include <cstdint>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <dxgi1_4.h>
#include <tbb/task_scheduler_init.h>
#include <windows.h>
#include <wrl.h>

#include <App/CommandListProcessor.h>
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

	__forceinline std::vector<std::unique_ptr<InitTask>>& InitTasks() noexcept { return mInitTasks; }
	__forceinline std::vector<std::unique_ptr<CmdBuilderTask>>& CmdBuilderTasks() noexcept { return mCmdBuilderTasks; }
	__forceinline ID3D12Device& Device() noexcept { ASSERT(mDevice.Get() != nullptr);  return *mDevice.Get(); }

protected:
	static App* mApp;	

	virtual void CreateRtvAndDsvDescriptorHeaps() noexcept;
	virtual void CreateRtvAndDsv() noexcept;
	virtual void Update(const float dt) noexcept;
	virtual void Draw(const float dt) noexcept;
	
	void InitSystems() noexcept;
	void InitMainWindow() noexcept;
	void InitDirect3D() noexcept;	
	void CreateCommandObjects() noexcept;
	void CreateSwapChain() noexcept;

	void FlushCommandQueue() noexcept;
	void SignalFenceAndPresent() noexcept;

	void CalculateFrameStats() noexcept;
	
	ID3D12Resource* CurrentBackBuffer() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const noexcept;
		
	tbb::task_scheduler_init mTaskSchedulerInit;
	CommandListProcessor* mCmdListProcessor{ nullptr };

	HINSTANCE mAppInst = nullptr; // application instance handle
	HWND mMainWnd = nullptr; // main window handle
	bool mAppPaused = false;  // is the application paused?

	Timer mTimer;

	std::vector<std::unique_ptr<InitTask>> mInitTasks;
	std::vector<std::unique_ptr<CmdBuilderTask>> mCmdBuilderTasks;

	Microsoft::WRL::ComPtr<IDXGIFactory4> mDxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
	Microsoft::WRL::ComPtr<ID3D12Device> mDevice;

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
	
	std::uint32_t mCurrBackBuffer{ 0U };
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[Settings::sSwapChainBufferCount];
	ID3D12Resource* mDepthStencilBuffer{ nullptr };

	ID3D12DescriptorHeap* mRtvHeap{ nullptr };
	ID3D12DescriptorHeap* mDsvHeap{ nullptr };	
};
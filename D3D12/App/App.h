#pragma once

#include <cstdint>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <dxgi1_4.h>
#include <tbb/task_scheduler_init.h>
#include <windows.h>
#include <wrl.h>

#include <App/CommandListProcessor.h>
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
	__forceinline const D3D12_VIEWPORT& Viewport() const noexcept { return mScreenViewport; }
	__forceinline const D3D12_RECT& ScissorRect() const noexcept { return mScissorRect; }

protected:
	static App* mApp;
	static const std::uint32_t sSwapChainBufferCount{ 2U };

	__forceinline float AspectRatio() const noexcept { return (float)mWindowWidth / mWindowHeight; }

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
	void FlushCommandQueueAndPresent() noexcept;

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
	ID3D12Fence* mFenceByFrameIndex[sSwapChainBufferCount]{ nullptr };
	std::uint64_t mCurrentFence{0U};

	ID3D12CommandQueue* mCmdQueue{ nullptr };
	ID3D12CommandAllocator* mDirectCmdAlloc1{ nullptr };
	ID3D12CommandAllocator* mDirectCmdAlloc2{ nullptr };
	ID3D12GraphicsCommandList* mCmdList1{ nullptr };
	ID3D12GraphicsCommandList* mCmdList2{ nullptr };
	
	std::uint32_t mCurrBackBuffer{0U};
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[sSwapChainBufferCount];
	ID3D12Resource* mDepthStencilBuffer{ nullptr };

	ID3D12DescriptorHeap* mRtvHeap{ nullptr };
	ID3D12DescriptorHeap* mDsvHeap{ nullptr };

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	// Derived class should set these in derived constructor to customize starting values.
	DXGI_FORMAT mBackBufferFormat{DXGI_FORMAT_R8G8B8A8_UNORM};
	DXGI_FORMAT mDepthStencilFormat{DXGI_FORMAT_D24_UNORM_S8_UINT};
	int32_t mWindowWidth{1920};
	int32_t mWindowHeight{1080};
};
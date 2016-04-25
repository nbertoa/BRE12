#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <cstdint>

#include <DXUtils\D3dUtils.h>
#include <Timer\Timer.h>

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class D3DApp {
protected:
	D3DApp(HINSTANCE hInstance);
	D3DApp(const D3DApp& rhs) = delete;
	D3DApp& operator=(const D3DApp& rhs) = delete;
	virtual ~D3DApp();

public:
	static D3DApp* GetApp();

	HINSTANCE AppInst() const;
	HWND MainWnd() const;
	float AspectRatio() const;

	bool Get4xMsaaState() const;
	void Set4xMsaaState(const bool value);

	int Run();

	virtual void Initialize();
	virtual LRESULT MsgProc(HWND hwnd, const int32_t msg, WPARAM wParam, LPARAM lParam);

protected:
	virtual void CreateRtvAndDsvDescriptorHeaps();
	virtual void OnResize();
	virtual void Update(const Timer& gt) = 0;
	virtual void Draw(const Timer& gt) = 0;

	// Convenience overrides for handling mouse input.
	virtual void OnMouseDown(const WPARAM /*btnState*/, const int32_t /*x*/, const int32_t /*y*/) { }
	virtual void OnMouseUp(const WPARAM /*btnState*/, const int32_t /*x*/, const int32_t /*y*/) { }
	virtual void OnMouseMove(const WPARAM /*btnState*/, const int32_t /*x*/, const int32_t /*y*/) { }

protected:
	void InitMainWindow();
	void InitDirect3D();
	void CreateCommandObjects();
	void CreateSwapChain();

	void FlushCommandQueue();

	ID3D12Resource* CurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

	void CalculateFrameStats();

	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter& adapter);
	void LogOutputDisplayModes(IDXGIOutput& output, const DXGI_FORMAT format);
	
protected:
	static D3DApp* mApp;

	HINSTANCE mAppInst = nullptr; // application instance handle
	HWND mMainWnd = nullptr; // main window handle
	bool mAppPaused = false;  // is the application paused?
	bool mMinimized = false;  // is the application minimized?
	bool mMaximized = false;  // is the application maximized?
	bool mResizing = false;   // are the resize bars being dragged?
	bool mFullscreenState = false; // fullscreen enabled
								  // Set true to use 4X MSAA (§4.1.8).  The default is false.
	bool m4xMsaaState = false;    // 4X MSAA enabled
	uint32_t m4xMsaaQuality = 0U;  // quality level of 4X MSAA

	Timer mTimer;

	Microsoft::WRL::ComPtr<IDXGIFactory4> mDxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
	Microsoft::WRL::ComPtr<ID3D12Device> mD3dDevice;

	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	uint64_t mCurrentFence = 0U;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

	static const uint32_t sSwapChainBufferCount = 2U;
	uint32_t mCurrBackBuffer = 0U;
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[sSwapChainBufferCount];
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	uint32_t mRtvDescSize = 0U;
	uint32_t mDsvDescSize = 0U;
	uint32_t mCbvSrvUavDescSize = 0U;
	uint32_t mSamplerDescSize = 0U;

	// Derived class should set these in derived constructor to customize starting values.
	std::wstring mMainWndCaption = L"D3d App";
	D3D_DRIVER_TYPE mD3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int32_t mClientWidth = 800;
	int32_t mClientHeight = 600;
};


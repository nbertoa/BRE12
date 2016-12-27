#include "D3dData.h"

#include <SettingsManager\SettingsManager.h>

namespace {
	void InitMainWindow(HWND& hwnd, const HINSTANCE hInstance) noexcept {
		WNDCLASS wc = {};
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = DefWindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = L"MainWnd";

		ASSERT(RegisterClass(&wc));

		// Compute window rectangle dimensions based on requested client area dimensions.
		RECT r = { 0, 0, static_cast<long>(SettingsManager::sWindowWidth), static_cast<long>(SettingsManager::sWindowHeight) };
		AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, false);
		const int32_t width{ r.right - r.left };
		const int32_t height{ r.bottom - r.top };

		const std::uint32_t dwStyle = SettingsManager::sFullscreen ? WS_POPUP : WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
		hwnd = CreateWindowEx(WS_EX_APPWINDOW, L"MainWnd", L"App", dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, hInstance, nullptr);
		ASSERT(hwnd);

		ShowWindow(hwnd, SW_SHOW);
		UpdateWindow(hwnd);
	}
}

HWND D3dData::mHwnd;
Microsoft::WRL::ComPtr<IDXGIFactory4> D3dData::mDxgiFactory{ nullptr };
Microsoft::WRL::ComPtr<ID3D12Device> D3dData::mDevice{ nullptr };

void D3dData::InitDirect3D(const HINSTANCE hInstance) noexcept {
	InitMainWindow(mHwnd, hInstance);

#if defined(DEBUG) || defined(_DEBUG) 
	// Enable the D3D12 debug layer.
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		CHECK_HR(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())));
		debugController->EnableDebugLayer();
	}
#endif

	// Create device
	CHECK_HR(CreateDXGIFactory1(IID_PPV_ARGS(D3dData::mDxgiFactory.GetAddressOf())));
	CHECK_HR(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(D3dData::mDevice.GetAddressOf())));
}
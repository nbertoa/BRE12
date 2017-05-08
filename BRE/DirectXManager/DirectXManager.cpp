#include "DirectXManager.h"

#include <SettingsManager\SettingsManager.h>

namespace BRE {
namespace {
void InitMainWindow(HWND& windowHandle, const HINSTANCE moduleInstanceHandle) noexcept
{
    WNDCLASS windowClass = {};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = DefWindowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = moduleInstanceHandle;
    windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
    windowClass.lpszMenuName = nullptr;
    windowClass.lpszClassName = L"MainWnd";

    BRE_ASSERT(RegisterClass(&windowClass));

    // Compute window rectangle dimensions based on requested client area dimensions.
    RECT rect = { 0, 0, static_cast<long>(SettingsManager::sWindowWidth), static_cast<long>(SettingsManager::sWindowHeight) };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
    const int32_t width{ rect.right - rect.left };
    const int32_t height{ rect.bottom - rect.top };

    const std::uint32_t windowStyle =
        SettingsManager::sIsFullscreenWindow ? WS_POPUP
        : WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    windowHandle = CreateWindowEx(
        WS_EX_APPWINDOW,
        L"MainWnd",
        L"App",
        windowStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        nullptr,
        nullptr,
        moduleInstanceHandle,
        nullptr);

    BRE_ASSERT(windowHandle);

    ShowWindow(windowHandle, SW_SHOW);
    UpdateWindow(windowHandle);
}
}

HWND DirectXManager::mWindowHandle;
Microsoft::WRL::ComPtr<IDXGIFactory4> DirectXManager::mDxgiFactory{ nullptr };
Microsoft::WRL::ComPtr<ID3D12Device> DirectXManager::mDevice{ nullptr };

void
DirectXManager::Init(const HINSTANCE moduleInstanceHandle) noexcept
{
    InitMainWindow(mWindowHandle, moduleInstanceHandle);

#if defined(DEBUG) || defined(_DEBUG) 
    // Enable the D3D12 debug layer.
    {
        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
        BRE_CHECK_HR(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())));
        debugController->EnableDebugLayer();
    }
#endif

    // Create device
    BRE_CHECK_HR(CreateDXGIFactory1(IID_PPV_ARGS(mDxgiFactory.GetAddressOf())));
    BRE_CHECK_HR(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(mDevice.GetAddressOf())));
}
}


#pragma once

#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>

#include <Utils\DebugUtils.h>

namespace BRE {
///
/// @brief Responsible to initialize and provide DirectX data
///
class DirectXManager {
public:
    DirectXManager() = delete;
    ~DirectXManager() = delete;
    DirectXManager(const DirectXManager&) = delete;
    const DirectXManager& operator=(const DirectXManager&) = delete;
    DirectXManager(DirectXManager&&) = delete;
    DirectXManager& operator=(DirectXManager&&) = delete;

    ///
    /// @brief Initializes window and DirectX device
    /// @param moduleInstanceHandle Module instance handle to create the window
    ///
    static void InitWindowAndDevice(const HINSTANCE moduleInstanceHandle) noexcept;

    ///
    /// @brief Get window handle
    /// #return Window handle
    ///
    __forceinline static HWND GetWindowHandle() noexcept
    {
        return mWindowHandle;
    }
    
    ///
    /// @brief Get DXGI Factory
    /// @return The DXGI Factory
    ///
    __forceinline static IDXGIFactory4& GetIDXGIFactory() noexcept
    {
        BRE_ASSERT(mDxgiFactory.Get() != nullptr);
        return *mDxgiFactory.Get();
    }

    ///
    /// @brief Get DirectX Device
    /// @return The DirectX Device
    ///
    __forceinline static ID3D12Device& GetDevice() noexcept
    {
        BRE_ASSERT(mDevice.Get() != nullptr);
        return *mDevice.Get();
    }

    ///
    /// @brief Get descriptor handle increment size
    /// @param descriptorHeapType The descriptor heap type
    /// @return The size of the descriptor handle according the heap type.
    ///
    __forceinline static std::size_t GetDescriptorHandleIncrementSize(const D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType)
    {
        BRE_ASSERT(mDevice.Get() != nullptr);
        return mDevice.Get()->GetDescriptorHandleIncrementSize(descriptorHeapType);
    }

private:
    static HWND mWindowHandle;
    static Microsoft::WRL::ComPtr<IDXGIFactory4> mDxgiFactory;
    static Microsoft::WRL::ComPtr<ID3D12Device> mDevice;
};

}


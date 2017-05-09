#pragma once

#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>

#include <Utils\DebugUtils.h>

namespace BRE {
// To initialize and get Direct3D data
class DirectXManager {
public:
    DirectXManager() = delete;
    ~DirectXManager() = delete;
    DirectXManager(const DirectXManager&) = delete;
    const DirectXManager& operator=(const DirectXManager&) = delete;
    DirectXManager(DirectXManager&&) = delete;
    DirectXManager& operator=(DirectXManager&&) = delete;

    static void Init(const HINSTANCE moduleInstanceHandle) noexcept;

    __forceinline static HWND GetWindowHandle() noexcept
    {
        return mWindowHandle;
    }
    __forceinline static IDXGIFactory4& GetIDXGIFactory() noexcept
    {
        BRE_ASSERT(mDxgiFactory.Get() != nullptr);
        return *mDxgiFactory.Get();
    }
    __forceinline static ID3D12Device& GetDevice() noexcept
    {
        BRE_ASSERT(mDevice.Get() != nullptr);
        return *mDevice.Get();
    }
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


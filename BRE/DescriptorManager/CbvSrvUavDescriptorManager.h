#pragma once

#include <d3d12.h>
#include <mutex>
#include <wrl.h>

#include <Utils/DebugUtils.h>

// To create Constant Buffer / Shader GetResource / Unordered Access Views descriptor heaps
// To create Constant Buffer / Shader GetResource / Unordered Access descriptors
class CbvSrvUavDescriptorManager {
public:
    CbvSrvUavDescriptorManager() = delete;
    ~CbvSrvUavDescriptorManager() = delete;
    CbvSrvUavDescriptorManager(const CbvSrvUavDescriptorManager&) = delete;
    const CbvSrvUavDescriptorManager& operator=(const CbvSrvUavDescriptorManager&) = delete;
    CbvSrvUavDescriptorManager(CbvSrvUavDescriptorManager&&) = delete;
    CbvSrvUavDescriptorManager& operator=(CbvSrvUavDescriptorManager&&) = delete;

    static void Init() noexcept;

    //
    // The following methods return GPU descriptor handle for the CBV, SRV or UAV.
    //
    // In the case of methods that creates several views, it returns the GPU desc handle
    // to the first element. As we guarantee all the other views are contiguous, then
    // you can easily build GPU desc handle for other view.
    //

    static D3D12_GPU_DESCRIPTOR_HANDLE CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& cBufferViewDescriptor) noexcept;

    // Preconditions:
    // - "descriptors" must not be nullptr
    // - "descriptorCount" must be greated than zero
    static D3D12_GPU_DESCRIPTOR_HANDLE CreateConstantBufferViews(const D3D12_CONSTANT_BUFFER_VIEW_DESC* descriptors,
                                                                 const std::uint32_t descriptorCount) noexcept;

    static D3D12_GPU_DESCRIPTOR_HANDLE CreateShaderResourceView(ID3D12Resource& resource,
                                                                const D3D12_SHADER_RESOURCE_VIEW_DESC& shaderResourceViewDescriptor) noexcept;

    // Preconditions:
    // - "resources" must not be nullptr
    // - "descriptors" must not be nullptr
    // - "descriptorCount" must be greated than zero
    static D3D12_GPU_DESCRIPTOR_HANDLE CreateShaderResourceViews(ID3D12Resource* *resources,
                                                                 const D3D12_SHADER_RESOURCE_VIEW_DESC* descriptors,
                                                                 const std::uint32_t descriptorCount) noexcept;

    static D3D12_GPU_DESCRIPTOR_HANDLE CreateUnorderedAccessView(ID3D12Resource& resource,
                                                                 const D3D12_UNORDERED_ACCESS_VIEW_DESC& descriptor) noexcept;

    // Preconditions:
    // - "resources" must not be nullptr
    // - "descriptors" must not be nullptr
    // - "descriptorCount" must be greated than zero
    static D3D12_GPU_DESCRIPTOR_HANDLE CreateUnorderedAccessViews(ID3D12Resource* *resources,
                                                                  const D3D12_UNORDERED_ACCESS_VIEW_DESC* descriptors,
                                                                  const std::uint32_t descriptorCount) noexcept;

    static ID3D12DescriptorHeap& GetDescriptorHeap() noexcept
    {
        ASSERT(mCbvSrvUavDescriptorHeap.Get() != nullptr);
        return *mCbvSrvUavDescriptorHeap.Get();
    }

private:
    static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvSrvUavDescriptorHeap;

    static D3D12_GPU_DESCRIPTOR_HANDLE mCurrentCbvSrvUavGpuDescriptorHandle;
    static D3D12_CPU_DESCRIPTOR_HANDLE mCurrentCbvSrvUavCpuDescriptorHandle;

    static std::mutex mMutex;
};

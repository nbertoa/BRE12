#pragma once

#include <d3d12.h>
#include <mutex>
#include <wrl.h>

#include <Utils/DebugUtils.h>

namespace BRE {
///
/// @brief Responsible to create constant buffers, shader resource views,
/// and unordered access views.
///
class CbvSrvUavDescriptorManager {
public:
    CbvSrvUavDescriptorManager() = delete;
    ~CbvSrvUavDescriptorManager() = delete;
    CbvSrvUavDescriptorManager(const CbvSrvUavDescriptorManager&) = delete;
    const CbvSrvUavDescriptorManager& operator=(const CbvSrvUavDescriptorManager&) = delete;
    CbvSrvUavDescriptorManager(CbvSrvUavDescriptorManager&&) = delete;
    CbvSrvUavDescriptorManager& operator=(CbvSrvUavDescriptorManager&&) = delete;

    ///
    /// @brief Initializes manager, for example, descriptor heap.
    /// @param numDescriptorsInCbvSrvUavDescriptorHeap Number of descriptors in
    /// descriptor heap of Constant Buffer Views, Shader Resource Views, and Unordered Access Views.
    ///
    static void Init(const std::uint32_t numDescriptorsInCbvSrvUavDescriptorHeap) noexcept;
    
    ///
    /// @brief Create a constant buffer view
    /// @param cBufferViewDescriptor Constant buffer view descriptor
    /// @return Gpu descriptor handle of the view
    ///
    static D3D12_GPU_DESCRIPTOR_HANDLE
        CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& cBufferViewDescriptor) noexcept;

    ///
    /// @brief Create constant buffer views
    /// @param descriptors Constant buffer views descriptors. It must not be nullptr.
    /// @param descriptorCount Number of constant buffer views descriptors. It must be greater than zero.
    /// @return The GPU descriptor handle to the first element. All the others views are contiguous,
    /// then you can easily build GPU descriptor handle for other view.
    ///
    static D3D12_GPU_DESCRIPTOR_HANDLE CreateConstantBufferViews(const D3D12_CONSTANT_BUFFER_VIEW_DESC* descriptors,
                                                                 const std::uint32_t descriptorCount) noexcept;

    ///
    /// @brief Create a shader resource view
    /// @param resource Resource to create the with to
    /// @param shaderResourceViewDescriptor The shader resource view descriptor
    /// @return The gpu descriptor handle for the view
    /// 
    static D3D12_GPU_DESCRIPTOR_HANDLE CreateShaderResourceView(ID3D12Resource& resource,
                                                                const D3D12_SHADER_RESOURCE_VIEW_DESC& shaderResourceViewDescriptor) noexcept;

    ///
    /// @brief Create shader resource views
    /// @param resources The list of resources to create views. It must not be nullptr.
    /// @param descriptors Shader resource views descriptors. It must not be nullptr.
    /// @param descriptorCount Number of shader resource views descriptors. It must be greater than zero.
    /// @return The GPU descriptor handle to the first element. All the others views are contiguous,
    /// then you can easily build GPU descriptor handle for other view.
    ///
    static D3D12_GPU_DESCRIPTOR_HANDLE CreateShaderResourceViews(ID3D12Resource* *resources,
                                                                 const D3D12_SHADER_RESOURCE_VIEW_DESC* descriptors,
                                                                 const std::uint32_t descriptorCount) noexcept;

    ///
    /// @brief Create unordered access view
    /// @param resource The resource to create the view.
    /// @param descriptor Unordered access view descriptor
    /// @return The GPU descriptor handle for the view
    ///
    static D3D12_GPU_DESCRIPTOR_HANDLE CreateUnorderedAccessView(ID3D12Resource& resource,
                                                                 const D3D12_UNORDERED_ACCESS_VIEW_DESC& descriptor) noexcept;

    ///
    /// @brief Create unordered access resource views
    /// @param resources The list of resources to create views. It must not be nullptr.
    /// @param descriptors Unordered access resource views descriptors. It must not be nullptr.
    /// @param descriptorCount Number of unordered access resource views descriptors. It must be greater than zero.
    /// @return The GPU descriptor handle to the first element. All the others views are contiguous,
    /// then you can easily build GPU descriptor handle for other view.
    ///
    static D3D12_GPU_DESCRIPTOR_HANDLE CreateUnorderedAccessViews(ID3D12Resource* *resources,
                                                                  const D3D12_UNORDERED_ACCESS_VIEW_DESC* descriptors,
                                                                  const std::uint32_t descriptorCount) noexcept;

    ///
    /// @brief Get descriptor heap
    /// @return The descriptor heap
    ///
    static ID3D12DescriptorHeap& GetDescriptorHeap() noexcept
    {
        BRE_ASSERT(mCbvSrvUavDescriptorHeap.Get() != nullptr);
        return *mCbvSrvUavDescriptorHeap.Get();
    }

private:
    static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvSrvUavDescriptorHeap;

    static D3D12_GPU_DESCRIPTOR_HANDLE mCurrentCbvSrvUavGpuDescriptorHandle;
    static D3D12_CPU_DESCRIPTOR_HANDLE mCurrentCbvSrvUavCpuDescriptorHandle;

    static std::mutex mMutex;
};

}


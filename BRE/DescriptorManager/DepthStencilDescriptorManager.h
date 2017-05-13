#pragma once

#include <d3d12.h>
#include <mutex>
#include <wrl.h>

#include <Utils/DebugUtils.h>

namespace BRE {
///
/// @brief Responsible to create depth stencil descriptors and heaps
///
class DepthStencilDescriptorManager {
public:
    DepthStencilDescriptorManager() = delete;
    ~DepthStencilDescriptorManager() = delete;
    DepthStencilDescriptorManager(const DepthStencilDescriptorManager&) = delete;
    const DepthStencilDescriptorManager& operator=(const DepthStencilDescriptorManager&) = delete;
    DepthStencilDescriptorManager(DepthStencilDescriptorManager&&) = delete;
    DepthStencilDescriptorManager& operator=(DepthStencilDescriptorManager&&) = delete;

    ///
    /// @brief Initializes the manager
    ///
    static void Init() noexcept;

    ///
    /// @brief Create a depth stencil view
    /// @param resource Resource to create the view to
    /// @param descriptor Depth stencil view descriptor
    /// @param cpuDescriptorHandle Optional parameter. It will store
    /// the CPU descriptor handle.
    /// @return The GPU descriptor handle to the view
    ///
    static D3D12_GPU_DESCRIPTOR_HANDLE CreateDepthStencilView(ID3D12Resource& resource,
                                                              const D3D12_DEPTH_STENCIL_VIEW_DESC& descriptor,
                                                              D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptorHandle = nullptr) noexcept;

    ///
    /// @brief Create depth stencil views
    /// @param resources The list of resources to create depth stencil views to them. It must not be nullptr.
    /// @param descriptors The list of depth stencil view descriptors. It must not be nullptr.
    /// @param descriptorCount The number of depth stencil view descriptors. It must be greater than zero.
    /// @param firstViewCpuDescriptorHandle Optional parameter. It will store the CPU descriptor handle to the first element.
    /// As we guarantee all the other views are contiguous, then
    /// you can easily build GPU descriptor handle for other views.
    /// @return It returns the GPU descriptor handle to the first element. 
    /// As we guarantee all the other views are contiguous, then you can easily build GPU descriptor handle for other view.
    ///
    static D3D12_GPU_DESCRIPTOR_HANDLE CreateDepthStencilViews(ID3D12Resource* *resources,
                                                               const D3D12_DEPTH_STENCIL_VIEW_DESC* descriptors,
                                                               const std::uint32_t descriptorCount,
                                                               D3D12_CPU_DESCRIPTOR_HANDLE* firstViewCpuDescriptorHandle = nullptr) noexcept;

private:
    static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDepthStencilViewDescriptorHeap;

    static D3D12_GPU_DESCRIPTOR_HANDLE mCurrentDepthStencilViewGpuDescriptorHandle;
    static D3D12_CPU_DESCRIPTOR_HANDLE mCurrentDepthStencilCpuDescriptorHandle;

    static std::mutex mMutex;
};

}


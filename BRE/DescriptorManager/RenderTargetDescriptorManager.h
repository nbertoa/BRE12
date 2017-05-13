#pragma once

#include <d3d12.h>
#include <mutex>
#include <wrl.h>

#include <Utils/DebugUtils.h>

namespace BRE {
// To create render target descriptor heaps
// To create render target descriptors
///
/// @brief Responsible to create render target descriptors and heaps.
///
class RenderTargetDescriptorManager {
public:
    RenderTargetDescriptorManager() = delete;
    ~RenderTargetDescriptorManager() = delete;
    RenderTargetDescriptorManager(const RenderTargetDescriptorManager&) = delete;
    const RenderTargetDescriptorManager& operator=(const RenderTargetDescriptorManager&) = delete;
    RenderTargetDescriptorManager(RenderTargetDescriptorManager&&) = delete;
    RenderTargetDescriptorManager& operator=(RenderTargetDescriptorManager&&) = delete;

    ///
    /// @brief Initializes the manager
    ///
    static void Init() noexcept;

    ///
    /// @brief Create a render target view
    /// @param resource Resource to create the view to
    /// @param descriptor Render target view descriptor
    /// @param cpuDescriptorHandle Optional parameter. It will store
    /// the CPU descriptor handle.
    /// @return The GPU descriptor handle to the view
    ///
    static D3D12_GPU_DESCRIPTOR_HANDLE CreateRenderTargetView(ID3D12Resource& resource,
                                                              const D3D12_RENDER_TARGET_VIEW_DESC& descriptor,
                                                              D3D12_CPU_DESCRIPTOR_HANDLE* firstViewCpuDescriptorHandle = nullptr) noexcept;

    ///
    /// @brief Create render target views
    /// @param resources The list of resources to create render target views to them. It must not be nullptr.
    /// @param descriptors The list of render target view descriptors. It must not be nullptr.
    /// @param descriptorCount The number of render target view descriptors. It must be greater than zero.
    /// @param firstViewCpuDescriptorHandle Optional parameter. It will store the CPU descriptor handle to the first element.
    /// As we guarantee all the other views are contiguous, then
    /// you can easily build GPU descriptor handle for other views.
    /// @return It returns the GPU descriptor handle to the first element. 
    /// As we guarantee all the other views are contiguous, then you can easily build GPU descriptor handle for other view.
    ///
    static D3D12_GPU_DESCRIPTOR_HANDLE CreateRenderTargetViews(ID3D12Resource* *resources,
                                                               const D3D12_RENDER_TARGET_VIEW_DESC* descriptors,
                                                               const std::uint32_t descriptorCount,
                                                               D3D12_CPU_DESCRIPTOR_HANDLE* firstViewCpuDescriptorHandle = nullptr) noexcept;

private:
    static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRenderTargetViewDescriptorHeap;

    static D3D12_GPU_DESCRIPTOR_HANDLE mCurrentRenderTargetViewDescriptorHandle;
    static D3D12_CPU_DESCRIPTOR_HANDLE mCurrentRenderTargetViewCpuDescriptorHandle;

    static std::mutex mMutex;
};

}


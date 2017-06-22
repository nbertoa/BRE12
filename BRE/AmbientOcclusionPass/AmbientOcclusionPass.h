#pragma once

#include <CommandManager\CommandListPerFrame.h>
#include <AmbientOcclusionPass\AmbientOcclusionCommandListRecorder.h>
#include <AmbientOcclusionPass\BlurCommandListRecorder.h>

namespace BRE {
///
/// @brief Pass responsible to generate ambient accessibility buffer (for ambient occlusion)
/// 
class AmbientOcclusionPass {
public:
    AmbientOcclusionPass() = default;
    ~AmbientOcclusionPass() = default;
    AmbientOcclusionPass(const AmbientOcclusionPass&) = delete;
    const AmbientOcclusionPass& operator=(const AmbientOcclusionPass&) = delete;
    AmbientOcclusionPass(AmbientOcclusionPass&&) = delete;
    AmbientOcclusionPass& operator=(AmbientOcclusionPass&&) = delete;

    ///
    /// @brief Initializes the pass
    /// @param normalRoughnessBuffer Geometry buffer that contains normals and roughness factors.
    /// @param depthBuffer Depth buffer
    /// @param normalRoughnessBufferShaderResourceView Shader resource view to
    /// the normal and roughness buffer
    /// @param depthBufferShaderResourceView Shader resource view to the depth buffer
    ///
    void Init(ID3D12Resource& normalRoughnessBuffer,
              ID3D12Resource& depthBuffer,             
              const D3D12_GPU_DESCRIPTOR_HANDLE& normalRoughnessBufferShaderResourceView,
              const D3D12_GPU_DESCRIPTOR_HANDLE& depthBufferShaderResourceView) noexcept;

    ///
    /// @brief Executes the pass
    ///
    /// Init() must be called first. This method can record and
    /// push command lists to the CommandListExecutor.
    ///
    /// @param frameCBuffer Constant buffer per frame, for current frame
    /// @return The number of recorded command lists.
    ///
    std::uint32_t Execute(const FrameCBuffer& frameCBuffer) noexcept;

    ///
    /// @brief Get the ambient accessibility buffer. This is necessary for other passes.
    /// @return Ambient accessibility buffer
    ///
    ID3D12Resource& GetAmbientAccessibilityBuffer() noexcept
    {
        BRE_ASSERT(mBlurBuffer != nullptr);
        return *mBlurBuffer;
    }

    ///
    /// @brief Get ambient accessibility buffer shader resource view. This is necessary for other passes
    /// @return Ambient accessibility buffer shader resource view
    ///
    D3D12_GPU_DESCRIPTOR_HANDLE GetAmbientAccessibilityShaderResourceView() noexcept
    {
        BRE_ASSERT(mBlurBufferShaderResourceView.ptr != 0UL);
        return mBlurBufferShaderResourceView;
    }

private:
    ///
    /// @brief Checks if internal data is valid. Typically, used for assertions
    /// @return True if valid. Otherwise, false
    ///
    bool IsDataValid() const noexcept;

    ///
    /// @brief Records pre pass command lists and pushes them to 
    /// the CommandListExecutor.
    /// @return The number of recorded command lists
    ///
    std::uint32_t RecordAndPushPrePassCommandLists() noexcept;

    ///
    /// @brief Records middle pass command lists and pushes them to 
    /// the CommandListExecutor.
    /// @return The number of recorded command lists
    ///
    std::uint32_t RecordAndPushMiddlePassCommandLists() noexcept;

    CommandListPerFrame mPrePassCommandListPerFrame;
    CommandListPerFrame mMiddlePassCommandListPerFrame;

    ID3D12Resource* mAmbientAccessibilityBuffer{ nullptr };
    D3D12_GPU_DESCRIPTOR_HANDLE mAmbientAccessibilityBufferShaderResourceView{ 0UL };
    D3D12_CPU_DESCRIPTOR_HANDLE mAmbientAccessibilityBufferRenderTargetView{ 0UL };

    ID3D12Resource* mBlurBuffer{ nullptr };
    D3D12_GPU_DESCRIPTOR_HANDLE mBlurBufferShaderResourceView{ 0UL };
    D3D12_CPU_DESCRIPTOR_HANDLE mBlurBufferRenderTargetView{ 0UL };

    AmbientOcclusionCommandListRecorder mAmbientOcclusionRecorder;
    BlurCommandListRecorder mBlurRecorder;

    ID3D12Resource* mNormalRoughnessBuffer{ nullptr };
    ID3D12Resource* mDepthBuffer{ nullptr };
};
}
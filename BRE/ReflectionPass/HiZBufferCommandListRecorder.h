#pragma once

#include <DirectXMath.h>

#include <CommandManager\CommandListPerFrame.h>
#include <ResourceManager\FrameUploadCBufferPerFrame.h>

namespace BRE {

///
/// @brief Responsible to record command lists to update a level
/// in the hi-z buffer.
///
class HiZBufferCommandListRecorder {
public:
    HiZBufferCommandListRecorder() = default;
    ~HiZBufferCommandListRecorder() = default;
    HiZBufferCommandListRecorder(const HiZBufferCommandListRecorder&) = delete;
    const HiZBufferCommandListRecorder& operator=(const HiZBufferCommandListRecorder&) = delete;
    HiZBufferCommandListRecorder(HiZBufferCommandListRecorder&&) = delete;
    HiZBufferCommandListRecorder& operator=(HiZBufferCommandListRecorder&&) = delete;

    ///
    /// @brief Initializes pipeline state object and root signature
    ///
    /// This method must be called at the beginning of the application, and once
    ///
    static void InitSharedPSOAndRootSignature() noexcept;

    ///
    /// @brief Initializes the command list recorder
    ///
    /// InitSharedPSOAndRootSignature() must be called first
    ///
    /// @param upperLevelBufferShaderResourceView Shader resource view to the upper level buffer in the hi-z buffer.
    /// This buffer is just before @p lowerLevelBufferRenderTargetView in the hi-z buffer
    /// (the only exception is at the top level, where the upper level buffer is the mip level 0 of the depth buffer,
    /// and the lower level buffer is the mip level 0 of the hi-z buffer)
    /// @param lowerLevelBufferRenderTargetView Render target view to the upper level buffer in the hi-z buffer.
    /// This buffer is just after @p upperLevelBufferShaderResourceView in the hi-z buffer
    /// (the only exception is at the top level, where the upper level buffer is the mip level 0 of the depth buffer,
    /// and the lower level buffer is the mip level 0 of the hi-z buffer)
    ///
    void Init(const D3D12_GPU_DESCRIPTOR_HANDLE& upperLevelBufferShaderResourceView,
              const D3D12_CPU_DESCRIPTOR_HANDLE& lowerLevelBufferRenderTargetView) noexcept;

    ///
    /// @brief Records and pushes command lists to CommandListExecutor
    ///
    /// Init() must be called first.
    ///
    /// @return The number of pushed command lists
    ///
    std::uint32_t RecordAndPushCommandLists() noexcept;

    ///
    /// @brief Checks if internal data is valid. Typically, used for assertions
    /// @return True if valid. Otherwise, false
    ///
    bool IsDataValid() const noexcept;

private:
    ///
    /// @brief Initializes shader resource views
    /// @param skyBoxCubeMap Sky box cube map resource
    ///
    void InitShaderResourceViews(ID3D12Resource& skyBoxCubeMap) noexcept;

    CommandListPerFrame mCommandListPerFrame;
    
    D3D12_GPU_DESCRIPTOR_HANDLE mUpperLevelBufferShaderResourceView{ 0UL };
    D3D12_CPU_DESCRIPTOR_HANDLE mLowerLevelBufferRenderTargetView{ 0UL };
};
}
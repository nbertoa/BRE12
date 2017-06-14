#pragma once

#include <DirectXMath.h>

#include <CommandManager\CommandListPerFrame.h>
#include <ResourceManager\FrameUploadCBufferPerFrame.h>

namespace BRE {
struct FrameCBuffer;

///
/// @brief Responsible to record command lists to update a level
/// in the visibility buffer.
///
class VisibilityBufferCommandListRecorder {
public:
    VisibilityBufferCommandListRecorder() = default;
    ~VisibilityBufferCommandListRecorder() = default;
    VisibilityBufferCommandListRecorder(const VisibilityBufferCommandListRecorder&) = delete;
    const VisibilityBufferCommandListRecorder& operator=(const VisibilityBufferCommandListRecorder&) = delete;
    VisibilityBufferCommandListRecorder(VisibilityBufferCommandListRecorder&&) = delete;
    VisibilityBufferCommandListRecorder& operator=(VisibilityBufferCommandListRecorder&&) = delete;

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
    /// @param upperLevelHiZBufferShaderResourceView Shader resource view to the upper level buffer in the hi-z buffer.
    /// @param lowerLevelHiZBufferShaderResourceView Shader resource view to the lower level buffer in the hi-z buffer.
    /// @param upperLevelVisibilityBufferShaderResourceView Shader resource view to the upper level buffer in the visibility buffer.
    /// This buffer is just before @p lowerLevelBufferRenderTargetView in the visibility buffer
    /// @param lowerLevelVisibilityBufferRenderTargetView Render target view to the upper level buffer in the visibility buffer.
    /// This buffer is just after @p upperLevelBufferShaderResourceView in the vsibility buffer
    ///
    void Init(const D3D12_GPU_DESCRIPTOR_HANDLE& upperLevelHiZBufferShaderResourceView,
              const D3D12_GPU_DESCRIPTOR_HANDLE& lowerLevelHiZBufferShaderResourceView,
              const D3D12_GPU_DESCRIPTOR_HANDLE& upperLevelVisibilityBufferShaderResourceView,
              const D3D12_CPU_DESCRIPTOR_HANDLE& lowerLevelVisibilityBufferRenderTargetView) noexcept;

    ///
    /// @brief Records and pushes command lists to CommandListExecutor
    ///
    /// Init() must be called first.
    ///
    /// @param frameCBuffer Constant buffer per frame, for current frame
    /// @return The number of pushed command lists
    ///
    std::uint32_t RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept;

    ///
    /// @brief Checks if internal data is valid. Typically, used for assertions
    /// @return True if valid. Otherwise, false
    ///
    bool IsDataValid() const noexcept;

private:
    CommandListPerFrame mCommandListPerFrame;

    FrameUploadCBufferPerFrame mFrameUploadCBufferPerFrame;

    D3D12_GPU_DESCRIPTOR_HANDLE mUpperLevelHiZBufferShaderResourceView{ 0UL };
    D3D12_GPU_DESCRIPTOR_HANDLE mLowerLevelHiZBufferShaderResourceView{ 0UL };
    D3D12_GPU_DESCRIPTOR_HANDLE mUpperLevelVisibilityBufferShaderResourceView{ 0UL };
    D3D12_CPU_DESCRIPTOR_HANDLE mLowerLevelVisibilityBufferRenderTargetView{ 0UL };
};
}
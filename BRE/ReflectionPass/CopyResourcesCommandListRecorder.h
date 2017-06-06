#pragma once

#include <DirectXMath.h>

#include <CommandManager\CommandListPerFrame.h>

namespace BRE {

///
/// @brief Responsible to copy the content from a input resource to the output resource.
///
class CopyResourcesCommandListRecorder {
public:
    CopyResourcesCommandListRecorder() = default;
    ~CopyResourcesCommandListRecorder() = default;
    CopyResourcesCommandListRecorder(const CopyResourcesCommandListRecorder&) = delete;
    const CopyResourcesCommandListRecorder& operator=(const CopyResourcesCommandListRecorder&) = delete;
    CopyResourcesCommandListRecorder(CopyResourcesCommandListRecorder&&) = delete;
    CopyResourcesCommandListRecorder& operator=(CopyResourcesCommandListRecorder&&) = delete;

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
    /// @param inputBufferShaderResourceView Shader resource view to the input buffer
    /// @param outputBufferRenderTargetView Render target view to the output buffer
    ///
    void Init(const D3D12_GPU_DESCRIPTOR_HANDLE& inputBufferShaderResourceView,
              const D3D12_CPU_DESCRIPTOR_HANDLE& outputBufferRenderTargetView) noexcept;

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
    CommandListPerFrame mCommandListPerFrame;

    D3D12_GPU_DESCRIPTOR_HANDLE mInputBufferShaderResourceView{ 0UL };
    D3D12_CPU_DESCRIPTOR_HANDLE mOutputBufferRenderTargetView{ 0UL };
};
}
#pragma once

#include <CommandManager\CommandListPerFrame.h>

namespace BRE {
///
/// Responsible to record and push command lists for the tone mapping pass
///
class ToneMappingCommandListRecorder {
public:
    ToneMappingCommandListRecorder() = default;
    ~ToneMappingCommandListRecorder() = default;
    ToneMappingCommandListRecorder(const ToneMappingCommandListRecorder&) = delete;
    const ToneMappingCommandListRecorder& operator=(const ToneMappingCommandListRecorder&) = delete;
    ToneMappingCommandListRecorder(ToneMappingCommandListRecorder&&) = default;
    ToneMappingCommandListRecorder& operator=(ToneMappingCommandListRecorder&&) = default;

    ///
    /// @brief Initializes pipeline state object and root signature
    ///
    /// This method must be called at the beginning of the application, and once
    ///
    static void InitSharedPSOAndRootSignature() noexcept;

    ///
    /// @brief Initializes the command list recorder
    ///
    /// InitSharedPSOAndRootSignature() must be called before
    ///
    /// @param inputColorBufferShaderResourceView Shader resource view to the input color buffer
    /// @param outputColorBufferRenderTargetView Render target view to the output color buffer
    ///
    void Init(const D3D12_GPU_DESCRIPTOR_HANDLE& inputColorBufferShaderResourceView,
              const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferRenderTargetView) noexcept;

    ///
    /// @brief Records and pushes command lists to the CommandListExecutor
    ///
    /// Init() must be called first
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

    D3D12_GPU_DESCRIPTOR_HANDLE mInputColorBufferShaderResourceView{ 0UL };
    D3D12_CPU_DESCRIPTOR_HANDLE mOutputColorBufferRenderTargetView{ 0UL };    
};
}
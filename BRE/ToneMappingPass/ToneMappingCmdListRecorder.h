#pragma once

#include <CommandManager\CommandListPerFrame.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;
struct ID3D12Resource;

namespace BRE {
///
/// Responsible to record command lists for the tone mapping pass
///
class ToneMappingCmdListRecorder {
public:
    ToneMappingCmdListRecorder() = default;
    ~ToneMappingCmdListRecorder() = default;
    ToneMappingCmdListRecorder(const ToneMappingCmdListRecorder&) = delete;
    const ToneMappingCmdListRecorder& operator=(const ToneMappingCmdListRecorder&) = delete;
    ToneMappingCmdListRecorder(ToneMappingCmdListRecorder&&) = default;
    ToneMappingCmdListRecorder& operator=(ToneMappingCmdListRecorder&&) = default;

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
    /// @param inputColorBuffer Input color buffer to apply the tone mapping
    /// @param renderTargetView Render target view
    ///
    void Init(ID3D12Resource& inputColorBuffer,
              const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

    ///
    /// @brief Records and pushes command lists to the CommandListExecutor
    ///
    /// Init() must be called first
    ///
    void RecordAndPushCommandLists() noexcept;

    ///
    /// @brief Checks if internal data is valid. Typically, used for assertions
    /// @return True if valid. Otherwise, false
    ///
    bool IsDataValid() const noexcept;

private:
    ///
    /// @brief Initializes shader resource views
    /// @param inputColorBuffer Input color buffer to apply tone mapping
    ///
    void InitShaderResourceViews(ID3D12Resource& inputColorBuffer) noexcept;

    CommandListPerFrame mCommandListPerFrame;

    D3D12_CPU_DESCRIPTOR_HANDLE mRenderTargetView{ 0UL };
    D3D12_GPU_DESCRIPTOR_HANDLE mStartPixelShaderResourceView{ 0UL };
};
}


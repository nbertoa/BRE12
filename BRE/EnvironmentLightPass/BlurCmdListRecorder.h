#pragma once

#include <CommandManager\CommandListPerFrame.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;
struct ID3D12Resource;

namespace BRE {
///
/// @brief Responsible of recording command lists that apply blur
///
class BlurCmdListRecorder {
public:
    BlurCmdListRecorder() = default;
    ~BlurCmdListRecorder() = default;
    BlurCmdListRecorder(const BlurCmdListRecorder&) = delete;
    const BlurCmdListRecorder& operator=(const BlurCmdListRecorder&) = delete;
    BlurCmdListRecorder(BlurCmdListRecorder&&) = default;
    BlurCmdListRecorder& operator=(BlurCmdListRecorder&&) = default;

    ///
    /// @brief Initializes pipeline state object and root signature
    ///
    /// This method must be called at the beginning of the application, and once
    ///
    static void InitSharedPSOAndRootSignature() noexcept;

    ///
    /// @brief Initialize the recorder
    ///
    /// This method must be called after InitSharedPSOAndRootSignature
    ///
    /// @param inputColorBuffer Input buffer that contains colors to be blurred
    /// @param renderTargetView
    ///
    void Init(ID3D12Resource& inputColorBuffer,
              const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

    ///
    /// @brief Records command lists and pushes them into CommandListExecutor
    ///
    /// Init() must be called first
    ///
    void RecordAndPushCommandLists() noexcept;

    ///
    /// @brief Validates internal data. Typically, used with assertions.
    ///
    bool ValidateData() const noexcept;

private:
    ///
    /// @brief Initializes shader resource views
    /// @param inputColorBuffer Input buffer that contains colors to be blurred
    ///
    void InitShaderResourceViews(ID3D12Resource& inputColorBuffer) noexcept;

    CommandListPerFrame mCommandListPerFrame;

    D3D12_GPU_DESCRIPTOR_HANDLE mStartPixelShaderResourceView{ 0UL };
    D3D12_CPU_DESCRIPTOR_HANDLE mRenderTargetView{ 0UL };
};
}


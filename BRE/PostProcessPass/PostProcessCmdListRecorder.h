#pragma once

#include <CommandManager\CommandListPerFrame.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;
struct ID3D12Resource;

namespace BRE {
///
/// Responsible to generate command list recorderes for post processing effects (anti aliasing, color grading, etc).
///
class PostProcessCmdListRecorder {
public:
    PostProcessCmdListRecorder() = default;
    ~PostProcessCmdListRecorder() = default;
    PostProcessCmdListRecorder(const PostProcessCmdListRecorder&) = delete;
    const PostProcessCmdListRecorder& operator=(const PostProcessCmdListRecorder&) = delete;
    PostProcessCmdListRecorder(PostProcessCmdListRecorder&&) = default;
    PostProcessCmdListRecorder& operator=(PostProcessCmdListRecorder&&) = default;

    ///
    /// @brief Initializes pipeline state object and root signature
    ///
    /// This method must be called at the beginning of application, and once.
    ///
    static void InitSharedPSOAndRootSignature() noexcept;

    ///
    /// @brief Initializes the command list recorder
    ///
    /// InitSharedPSOAndRootSignature() must be called before
    ///
    /// @param inputColorBuffer Input color buffer
    ///
    void Init(ID3D12Resource& inputColorBuffer) noexcept;

    ///
    /// @brief Records and pushes command lists to CommandListExecutor
    ///
    /// Init() must be called first
    ///
    /// @param renderTargetView Render target view
    ///
    void RecordAndPushCommandLists(const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

    ///
    /// @brief Checks if internal data is valid. Typically, used with assertions
    /// @return True if valid. Otherwise, false
    ///
    bool IsDataValid() const noexcept;

private:
    ///
    /// @brief Initializes shader resource views
    /// @param inputColorBuffer Input color buffer
    ///
    void InitShaderResourceViews(ID3D12Resource& inputColorBuffer) noexcept;

    CommandListPerFrame mCommandListPerFrame;

    D3D12_GPU_DESCRIPTOR_HANDLE mStartPixelShaderResourceView{ 0UL };
};
}


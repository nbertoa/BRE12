#pragma once

#include <CommandManager\CommandListPerFrame.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;
struct ID3D12Resource;

namespace BRE {
///
/// Responsible to record and push command lists for post processing effects (anti aliasing, color grading, etc).
///
class PostProcessCommandListRecorder {
public:
    PostProcessCommandListRecorder() = default;
    ~PostProcessCommandListRecorder() = default;
    PostProcessCommandListRecorder(const PostProcessCommandListRecorder&) = delete;
    const PostProcessCommandListRecorder& operator=(const PostProcessCommandListRecorder&) = delete;
    PostProcessCommandListRecorder(PostProcessCommandListRecorder&&) = default;
    PostProcessCommandListRecorder& operator=(PostProcessCommandListRecorder&&) = default;

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
    /// @param inputColorBufferShaderResourceView Shader resource view to the input color buffer
    ///
    void Init(const D3D12_GPU_DESCRIPTOR_HANDLE& inputColorBufferShaderResourceView) noexcept;

    ///
    /// @brief Records and pushes command lists to CommandListExecutor
    ///
    /// Init() must be called first
    ///
    /// @param outputColorBufferRenderTargetView Render target view to the output color buffer
    /// @return The number of pushed command lists
    ///
    std::uint32_t RecordAndPushCommandLists(const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferRenderTargetView) noexcept;

    ///
    /// @brief Checks if internal data is valid. Typically, used with assertions
    /// @return True if valid. Otherwise, false
    ///
    bool IsDataValid() const noexcept;

private:
    CommandListPerFrame mCommandListPerFrame;

    D3D12_GPU_DESCRIPTOR_HANDLE mInputColorBufferShaderResourceView{ 0UL };
};
}
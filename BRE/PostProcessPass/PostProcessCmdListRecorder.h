#pragma once

#include <CommandManager\CommandListPerFrame.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;
struct ID3D12Resource;

namespace BRE {
// To record command list for post processing effects (anti aliasing, color grading, etc).
class PostProcessCmdListRecorder {
public:
    PostProcessCmdListRecorder() = default;
    ~PostProcessCmdListRecorder() = default;
    PostProcessCmdListRecorder(const PostProcessCmdListRecorder&) = delete;
    const PostProcessCmdListRecorder& operator=(const PostProcessCmdListRecorder&) = delete;
    PostProcessCmdListRecorder(PostProcessCmdListRecorder&&) = default;
    PostProcessCmdListRecorder& operator=(PostProcessCmdListRecorder&&) = default;

    static void InitSharedPSOAndRootSignature() noexcept;

    // Preconditions:
    // - InitSharedPSOAndRootSignature() must be called first and once
    void Init(ID3D12Resource& inputColorBuffer) noexcept;

    // Preconditions:
    // - Init() must be called first
    void RecordAndPushCommandLists(const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

    bool IsDataValid() const noexcept;

private:
    void InitShaderResourceViews(ID3D12Resource& inputColorBuffer) noexcept;

    CommandListPerFrame mCommandListPerFrame;

    D3D12_GPU_DESCRIPTOR_HANDLE mStartPixelShaderResourceView{ 0UL };
};
}


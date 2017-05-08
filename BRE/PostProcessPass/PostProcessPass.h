#pragma once

#include <memory>

#include <CommandManager\CommandListPerFrame.h>
#include <PostProcessPass\PostProcessCmdListRecorder.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct ID3D12Resource;

// Pass that applies post processing effects (anti aliasing, color grading, etc)
class PostProcessPass {
public:
    PostProcessPass() = default;
    ~PostProcessPass() = default;
    PostProcessPass(const PostProcessPass&) = delete;
    const PostProcessPass& operator=(const PostProcessPass&) = delete;
    PostProcessPass(PostProcessPass&&) = delete;
    PostProcessPass& operator=(PostProcessPass&&) = delete;

    void Init(ID3D12Resource& inputColorBuffer) noexcept;

    // Preconditions:
    // - Init() must be called first
    void Execute(ID3D12Resource& renderTargetBuffer,
                 const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

private:
    // Method used internally for validation purposes
    bool IsDataValid() const noexcept;

    void ExecuteBeginTask(ID3D12Resource& renderTargetBuffer,
                          const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

    CommandListPerFrame mCommandListPerFrame;

    ID3D12Resource* mInputColorBuffer{ nullptr };

    std::unique_ptr<PostProcessCmdListRecorder> mCommandListRecorder;
};

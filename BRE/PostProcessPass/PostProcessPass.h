#pragma once

#include <memory>

#include <CommandManager\CommandListPerFrame.h>
#include <PostProcessPass\PostProcessCommandListRecorder.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct ID3D12Resource;

namespace BRE {
///
/// @brief Pass responsible to apply post processing effects (anti aliasing, color grading, etc)
///
class PostProcessPass {
public:
    PostProcessPass() = default;
    ~PostProcessPass() = default;
    PostProcessPass(const PostProcessPass&) = delete;
    const PostProcessPass& operator=(const PostProcessPass&) = delete;
    PostProcessPass(PostProcessPass&&) = delete;
    PostProcessPass& operator=(PostProcessPass&&) = delete;

    ///
    /// @brief Initializes post process pass
    /// @param inputColorBuffer Input color buffer to apply post processing 
    ///
    void Init(ID3D12Resource& inputColorBuffer) noexcept;

    ///
    /// @brief Execute post process pass
    ///
    /// Init() must be called first
    ///
    /// @param renderTargetBuffer Render target buffer
    /// @param renderTargetView Render target view
    ///
    void Execute(ID3D12Resource& renderTargetBuffer,
                 const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

private:
    ///
    /// @brief Checks if internal data is valid. Typically, used with assertions
    /// @return True if valid. Otherwise, false.
    ///
    bool IsDataValid() const noexcept;

    ///
    /// @brief Execute begin task for pass
    /// @param renderTargetBuffer Render target buffer
    /// @param renderTargetView Render target view
    ///
    void ExecuteBeginTask(ID3D12Resource& renderTargetBuffer,
                          const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

    CommandListPerFrame mCommandListPerFrame;

    ID3D12Resource* mInputColorBuffer{ nullptr };

    std::unique_ptr<PostProcessCommandListRecorder> mCommandListRecorder;
};

}


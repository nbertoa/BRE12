#pragma once

#include <memory>

#include <CommandManager\CommandListPerFrame.h>
#include <PostProcessPass\PostProcessCommandListRecorder.h>

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
    /// @brief Executes the pass
    ///
    /// Init() must be called first. This method can record and
    /// push command lists to the CommandListExecutor.
    ///
    /// @param renderTargetBuffer Render target buffer
    /// @param renderTargetView Render target view
    /// @return The number of recorded command lists.
    ///
    std::uint32_t Execute(ID3D12Resource& renderTargetBuffer,
                          const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

private:
    ///
    /// @brief Checks if internal data is valid. Typically, used with assertions
    /// @return True if valid. Otherwise, false.
    ///
    bool IsDataValid() const noexcept;

    ///
    /// @brief Records pre pass command lists and pushes them to 
    /// the CommandListExecutor.
    /// @param renderTargetBuffer Render target buffer
    /// @param renderTargetView Render target view
    /// @return The number of recorded command lists
    ///
    std::uint32_t RecordAndPushPrePassCommandLists(ID3D12Resource& renderTargetBuffer,
                                                   const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

    CommandListPerFrame mPrePassCommandListPerFrame;

    ID3D12Resource* mInputColorBuffer{ nullptr };

    std::unique_ptr<PostProcessCommandListRecorder> mCommandListRecorder;
};
}
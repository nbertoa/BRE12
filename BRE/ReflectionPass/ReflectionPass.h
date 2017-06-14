#pragma once

#include <CommandManager\CommandListPerFrame.h>
#include <ReflectionPass\CopyResourcesCommandListRecorder.h>
#include <ReflectionPass\HiZBufferCommandListRecorder.h>
#include <ReflectionPass\VisibilityBufferCommandListRecorder.h>

namespace BRE {
///
/// @brief Pass responsible to apply hi-Z screen space cone-traced reflections
/// 
class ReflectionPass {
public:
    ReflectionPass() = default;
    ~ReflectionPass() = default;
    ReflectionPass(const ReflectionPass&) = delete;
    const ReflectionPass& operator=(const ReflectionPass&) = delete;
    ReflectionPass(ReflectionPass&&) = delete;
    ReflectionPass& operator=(ReflectionPass&&) = delete;

    ///
    /// @brief Initializes the pass
    /// @param depthBuffer Depth buffer
    ///
    void Init(ID3D12Resource& depthBuffer) noexcept;

    ///
    /// @brief Executes the pass
    ///
    /// Init() must be called first. This method can record and
    /// push command lists to the CommandListExecutor.
    ///
    /// @return The number of recorded command lists.
    ///
    std::uint32_t Execute() noexcept;

private:
    ///
    /// @brief Initializes hierarchy z buffer
    ///
    void InitHierZBuffer() noexcept;

    ///
    /// @brief Initializes visibility buffer
    ///
    void InitVisibilityBuffer() noexcept;

    ///
    /// @brief Checks if internal data is valid. Typically, used for assertions
    /// @return True if valid. Otherwise, false
    ///
    bool IsDataValid() const noexcept;

    ///
    /// @brief Records pre pass command lists and pushes them to 
    /// the CommandListExecutor.
    /// @return The number of recorded command lists
    ///
    std::uint32_t RecordAndPushPrePassCommandLists() noexcept;

    ///
    /// @brief Records command lists related with hi-z buffer and
    /// pushes them to the CommandListExecutor
    ///
    std::uint32_t RecordAndPushHierZBufferCommandLists() noexcept;

    ///
    /// @brief Records command lists related with the visibility buffer and
    /// pushes them to the CommandListExecutor
    ///
    std::uint32_t RecordAndPushVisibilityBufferCommandLists() noexcept;

    CommandListPerFrame mPrePassCommandListPerFrame;

    ID3D12Resource* mHierZBuffer{ nullptr };
    D3D12_CPU_DESCRIPTOR_HANDLE mHierZBufferMipLevelRenderTargetViews[10U]{ 0UL };
    D3D12_GPU_DESCRIPTOR_HANDLE mHierZBufferMipLevelShaderResourceViews[10U]{ 0UL };

    ID3D12Resource* mVisibilityBuffer{ nullptr };
    D3D12_CPU_DESCRIPTOR_HANDLE mVisibilityBufferMipLevelRenderTargetViews[10U]{ 0UL };
    D3D12_GPU_DESCRIPTOR_HANDLE mVisibilityBufferMipLevelShaderResourceViews[10U]{ 0UL };

    ID3D12Resource* mDepthBuffer{ nullptr };
    D3D12_GPU_DESCRIPTOR_HANDLE mDepthBufferShaderResourceView{ 0UL };

    CopyResourcesCommandListRecorder mCopyDepthBufferToHiZBufferMipLevel0CommandListRecorder;
    HiZBufferCommandListRecorder mHiZBufferCommandListRecorders[9U];
    VisibilityBufferCommandListRecorder mVisibilityBufferCommandListRecorders[9U];
};
}
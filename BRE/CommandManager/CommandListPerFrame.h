#pragma once

#include <cstdint>

#include <SettingsManager\SettingsManager.h>
#include <Utils\DebugUtils.h>

struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;
struct ID3D12PipelinesState;

namespace BRE {
///
/// @brief Provides support of command lists for different frames.
///
/// We support to have different number of queued frames.
/// This class provides a command list that can be reset with 
/// a different command allocator per queued frame.
///
class CommandListPerFrame {
public:
    CommandListPerFrame();
    ~CommandListPerFrame() = default;
    CommandListPerFrame(const CommandListPerFrame&) = delete;
    const CommandListPerFrame& operator=(const CommandListPerFrame&) = delete;
    CommandListPerFrame(CommandListPerFrame&&) = default;
    CommandListPerFrame& operator=(CommandListPerFrame&&) = default;

    ///
    /// @brief Reset command list with the command allocator for the next frame
    /// @param pso Pipeline state object to reset the command list
    /// @return The reset command list
    ///
    ID3D12GraphicsCommandList& ResetCommandListWithNextCommandAllocator(ID3D12PipelineState* pso) noexcept;

    ///
    /// @brief Get the command list
    /// @return The command list
    ///
    __forceinline ID3D12GraphicsCommandList& GetCommandList() noexcept
    {
        BRE_ASSERT(mCommandList != nullptr);
        return *mCommandList;
    }

private:
    ID3D12CommandAllocator* mCommandAllocators[SettingsManager::sQueuedFrameCount]{ nullptr };
    ID3D12GraphicsCommandList* mCommandList{ nullptr };
    std::uint32_t mCurrentFrameIndex{ 0U };
};
}


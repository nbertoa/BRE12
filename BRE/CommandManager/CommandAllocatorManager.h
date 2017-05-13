#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb/concurrent_unordered_set.h>

namespace BRE {
///
/// Class to create command allocators
///
class CommandAllocatorManager {
public:
    CommandAllocatorManager() = delete;
    ~CommandAllocatorManager() = delete;
    CommandAllocatorManager(const CommandAllocatorManager&) = delete;
    const CommandAllocatorManager& operator=(const CommandAllocatorManager&) = delete;
    CommandAllocatorManager(CommandAllocatorManager&&) = delete;
    CommandAllocatorManager& operator=(CommandAllocatorManager&&) = delete;

    ///
    /// @brief Releases all ID3D12CommandAllocator's
    ///
    static void Clear() noexcept;

    ///
    /// @brief Create a ID3D12CommandAllocator
    /// @param commandListType The type of the command list for this ID3D12CommandAllocator
    ///
    static ID3D12CommandAllocator& CreateCommandAllocator(const D3D12_COMMAND_LIST_TYPE& commandListType) noexcept;

private:
    static tbb::concurrent_unordered_set<ID3D12CommandAllocator*> mCommandAllocators;

    static std::mutex mMutex;
};
}

#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb/concurrent_unordered_set.h>

namespace BRE {
///
/// @brief Class responsible to create command lists.
///
class CommandListManager {
public:
    CommandListManager() = delete;
    ~CommandListManager() = delete;
    CommandListManager(const CommandListManager&) = delete;
    const CommandListManager& operator=(const CommandListManager&) = delete;
    CommandListManager(CommandListManager&&) = delete;
    CommandListManager& operator=(CommandListManager&&) = delete;

    ///
    /// @brief Releases all command lists
    ///
    static void Clear() noexcept;

    ///
    /// @brief Create command list
    /// @param commandListType Type of the command list to create
    /// @param commandAllocator Command allocator to use to create the command list
    /// @return The created command list
    ///
    static ID3D12GraphicsCommandList& CreateCommandList(const D3D12_COMMAND_LIST_TYPE& commandListType,
                                                        ID3D12CommandAllocator& commandAllocator) noexcept;

private:
    static tbb::concurrent_unordered_set<ID3D12GraphicsCommandList*> mCommandLists;

    static std::mutex mMutex;
};

}
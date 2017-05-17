#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb/concurrent_unordered_set.h>

namespace BRE {
///
/// @brief Responsible for command queue creation
///
class CommandQueueManager {
public:
    CommandQueueManager() = delete;
    ~CommandQueueManager() = delete;
    CommandQueueManager(const CommandQueueManager&) = delete;
    const CommandQueueManager& operator=(const CommandQueueManager&) = delete;
    CommandQueueManager(CommandQueueManager&&) = delete;
    CommandQueueManager& operator=(CommandQueueManager&&) = delete;

    ///
    /// @brief Release all command queues
    ///
    static void Clear() noexcept;

    ///
    /// @brief Create a command queue
    /// @param descriptor Descriptor of the command queue
    /// @return The created command queue
    ///
    static ID3D12CommandQueue& CreateCommandQueue(const D3D12_COMMAND_QUEUE_DESC& descriptor) noexcept;

private:
    static tbb::concurrent_unordered_set<ID3D12CommandQueue*> mCommandQueues;

    static std::mutex mMutex;
};
}
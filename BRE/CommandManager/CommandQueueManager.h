#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb/concurrent_unordered_set.h>

// To create/get command queues
class CommandQueueManager {
public:
	CommandQueueManager() = delete;
	~CommandQueueManager() = delete;
	CommandQueueManager(const CommandQueueManager&) = delete;
	const CommandQueueManager& operator=(const CommandQueueManager&) = delete;
	CommandQueueManager(CommandQueueManager&&) = delete;
	CommandQueueManager& operator=(CommandQueueManager&&) = delete;

	static void EraseAll() noexcept;

	static ID3D12CommandQueue& CreateCommandQueue(const D3D12_COMMAND_QUEUE_DESC& descriptor) noexcept;

private:
	using CommandQueues = tbb::concurrent_unordered_set<ID3D12CommandQueue*>;
	static CommandQueues mCommandQueues;

	static std::mutex mMutex;
};

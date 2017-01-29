#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb/concurrent_hash_map.h>
#include <wrl.h>

// To create/get command queues
class CommandQueueManager {
public:
	CommandQueueManager() = delete;
	~CommandQueueManager() = delete;
	CommandQueueManager(const CommandQueueManager&) = delete;
	const CommandQueueManager& operator=(const CommandQueueManager&) = delete;
	CommandQueueManager(CommandQueueManager&&) = delete;
	CommandQueueManager& operator=(CommandQueueManager&&) = delete;

	// Returns id to get the data after creation.
	static std::size_t CreateCommandQueue(
		const D3D12_COMMAND_QUEUE_DESC& descriptor, 
		ID3D12CommandQueue* &cmdQueue) noexcept;

	// Preconditions:
	// - "id" must be valid
	static ID3D12CommandQueue& GetCommandQueue(const std::size_t id) noexcept;

private:
	using CommandQueueById = tbb::concurrent_hash_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12CommandQueue>>;
	static CommandQueueById mCommandQueueById;

	static std::mutex mMutex;
};

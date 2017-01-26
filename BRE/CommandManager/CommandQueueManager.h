#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb/concurrent_hash_map.h>
#include <wrl.h>

// To create/get command queues
class CommandQueueManager {
public:
	// Preconditions:
	// - Create() must be called once
	static CommandQueueManager& Create() noexcept;

	// Preconditions:
	// - Create() must be called before this method
	static CommandQueueManager& Get() noexcept;

	~CommandQueueManager() = default;
	CommandQueueManager(const CommandQueueManager&) = delete;
	const CommandQueueManager& operator=(const CommandQueueManager&) = delete;
	CommandQueueManager(CommandQueueManager&&) = delete;
	CommandQueueManager& operator=(CommandQueueManager&&) = delete;

	// Returns id to get the data after creation.
	std::size_t CreateCommandQueue(const D3D12_COMMAND_QUEUE_DESC& descriptor, ID3D12CommandQueue* &cmdQueue) noexcept;

	// Preconditions:
	// - "id" must be valid
	ID3D12CommandQueue& GetCommandQueue(const std::size_t id) noexcept;

private:
	CommandQueueManager() = default;

	using CommandQueueById = tbb::concurrent_hash_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12CommandQueue>>;
	CommandQueueById mCommandQueueById;

	std::mutex mMutex;
};

#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb/concurrent_hash_map.h>
#include <wrl.h>

// To create/get command allocators
class CommandAllocatorManager {
public:
	// Preconditions:
	// - Create() must be called once
	static CommandAllocatorManager& Create() noexcept;

	// Preconditions:
	// - Create() must be called before this method
	static CommandAllocatorManager& Get() noexcept;

	~CommandAllocatorManager() = default;
	CommandAllocatorManager(const CommandAllocatorManager&) = delete;
	const CommandAllocatorManager& operator=(const CommandAllocatorManager&) = delete;
	CommandAllocatorManager(CommandAllocatorManager&&) = delete;
	CommandAllocatorManager& operator=(CommandAllocatorManager&&) = delete;

	// Returns id to get the data after creation.
	std::size_t CreateCommandAllocator(const D3D12_COMMAND_LIST_TYPE& commandListType, ID3D12CommandAllocator* &cmdListAllocator) noexcept;

	// Preconditions:
	// - "id" must be valid
	ID3D12CommandAllocator& GetCommandAllocator(const std::size_t id) noexcept;

private:
	CommandAllocatorManager() = default;

	using CommandAllocatorById = tbb::concurrent_hash_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>;
	CommandAllocatorById mCommandAllocatorById;

	std::mutex mMutex;
};

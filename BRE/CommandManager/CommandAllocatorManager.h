#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb/concurrent_unordered_set.h>

// To create/get command allocators
class CommandAllocatorManager {
public:
	CommandAllocatorManager() = delete;
	~CommandAllocatorManager();
	CommandAllocatorManager(const CommandAllocatorManager&) = delete;
	const CommandAllocatorManager& operator=(const CommandAllocatorManager&) = delete;
	CommandAllocatorManager(CommandAllocatorManager&&) = delete;
	CommandAllocatorManager& operator=(CommandAllocatorManager&&) = delete;

	static ID3D12CommandAllocator& CreateCommandAllocator(const D3D12_COMMAND_LIST_TYPE& commandListType) noexcept;

private:
	using CommandAllocators = tbb::concurrent_unordered_set<ID3D12CommandAllocator*>;
	static CommandAllocators mCommandAllocators;

	static std::mutex mMutex;
};

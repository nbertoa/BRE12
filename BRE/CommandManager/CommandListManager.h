#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb/concurrent_unordered_set.h>

// To create/get command lists
class CommandListManager {
public:
	CommandListManager() = delete;
	~CommandListManager();
	CommandListManager(const CommandListManager&) = delete;
	const CommandListManager& operator=(const CommandListManager&) = delete;
	CommandListManager(CommandListManager&&) = delete;
	CommandListManager& operator=(CommandListManager&&) = delete;

	static ID3D12GraphicsCommandList& CreateCommandList(
		const D3D12_COMMAND_LIST_TYPE& commandListType,
		ID3D12CommandAllocator& commandAllocator) noexcept;

private:
	using CommandLists = tbb::concurrent_unordered_set<ID3D12GraphicsCommandList*>;
	static CommandLists mCommandLists;

	static std::mutex mMutex;
};

#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb/concurrent_hash_map.h>
#include <wrl.h>

// To create/get command lists
class CommandListManager {
public:
	CommandListManager() = delete;
	~CommandListManager() = delete;
	CommandListManager(const CommandListManager&) = delete;
	const CommandListManager& operator=(const CommandListManager&) = delete;
	CommandListManager(CommandListManager&&) = delete;
	CommandListManager& operator=(CommandListManager&&) = delete;

	// Returns id to get the data after creation.
	static std::size_t CreateCommandList(
		const D3D12_COMMAND_LIST_TYPE& commandListType,
		ID3D12CommandAllocator& commandAllocator,
		ID3D12GraphicsCommandList* &commandList) noexcept;

	// Preconditions:
	// - "id" must be valid
	static ID3D12GraphicsCommandList& GetCommandList(const std::size_t id) noexcept;

private:
	using CommandListById = tbb::concurrent_hash_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>>;
	static CommandListById mCommandListById;

	static std::mutex mMutex;
};

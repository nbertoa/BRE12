#pragma once

#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <tbb/concurrent_hash_map.h>
#include <tbb/mutex.h>
#include <wrl.h>

class CommandManager {
public:
	static std::unique_ptr<CommandManager> gManager;

	explicit CommandManager(ID3D12Device& device) : mDevice(device) {}
	CommandManager(const CommandManager&) = delete;
	const CommandManager& operator=(const CommandManager&) = delete;

	std::size_t CreateCmdList(const D3D12_COMMAND_LIST_TYPE& type, ID3D12CommandAllocator& cmdAlloc, ID3D12GraphicsCommandList* &cmdList) noexcept;
	std::size_t CreateCmdAlloc(const D3D12_COMMAND_LIST_TYPE& type, ID3D12CommandAllocator* &cmdListAlloc) noexcept;

	// Asserts if resource id is not present
	ID3D12GraphicsCommandList& GetCmdList(const std::size_t id) noexcept;
	ID3D12CommandAllocator& GetCmdAlloc(const std::size_t id) noexcept;

	// This will invalidate all ids.
	__forceinline void ClearCmdLists() noexcept { mCmdListById.clear(); }
	__forceinline void ClearCmdAllocs() noexcept { mCmdAllocById.clear(); }
	__forceinline void Clear() noexcept { ClearCmdLists(); ClearCmdAllocs(); }

private:
	ID3D12Device& mDevice;

	using CmdListById = tbb::concurrent_hash_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>>;
	CmdListById mCmdListById;

	using CmdAllocById = tbb::concurrent_hash_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>;
	CmdAllocById mCmdAllocById;

	tbb::mutex mMutex;
};

#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb/concurrent_hash_map.h>
#include <wrl.h>

// To create/get cfences
class FenceManager {
public:
	FenceManager() = delete;
	~FenceManager() = delete;
	FenceManager(const FenceManager&) = delete;
	const FenceManager& operator=(const FenceManager&) = delete;
	FenceManager(FenceManager&&) = delete;
	FenceManager& operator=(FenceManager&&) = delete;

	// Returns id to get the data after creation.
	static std::size_t CreateFence(
		const std::uint64_t fenceInitialValue,
		const D3D12_FENCE_FLAGS& flags,
		ID3D12Fence* &fence) noexcept;

	// Preconditions:
	// - "id" must be valid
	static ID3D12Fence& GetFence(const std::size_t id) noexcept;

private:
	using FenceById = tbb::concurrent_hash_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12Fence>>;
	static FenceById mFenceById;

	static std::mutex mMutex;
};

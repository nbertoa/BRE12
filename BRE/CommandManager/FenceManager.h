#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb/concurrent_hash_map.h>
#include <wrl.h>

// To create/get cfences
class FenceManager {
public:
	// Preconditions:
	// - Create() must be called once
	static FenceManager& Create() noexcept;

	// Preconditions:
	// - Create() must be called before this method
	static FenceManager& Get() noexcept;

	~FenceManager() = default;
	FenceManager(const FenceManager&) = delete;
	const FenceManager& operator=(const FenceManager&) = delete;
	FenceManager(FenceManager&&) = delete;
	FenceManager& operator=(FenceManager&&) = delete;

	// Returns id to get the data after creation.
	std::size_t CreateFence(
		const std::uint64_t fenceInitialValue,
		const D3D12_FENCE_FLAGS& flags,
		ID3D12Fence* &fence) noexcept;

	// Preconditions:
	// - "id" must be valid
	ID3D12Fence& GetFence(const std::size_t id) noexcept;

private:
	FenceManager() = default;

	using FenceById = tbb::concurrent_hash_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12Fence>>;
	FenceById mFenceById;

	std::mutex mMutex;
};

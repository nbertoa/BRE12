#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb\concurrent_unordered_set.h>


// To create/get cfences
class FenceManager {
public:
	FenceManager() = delete;
	~FenceManager() = delete;
	FenceManager(const FenceManager&) = delete;
	const FenceManager& operator=(const FenceManager&) = delete;
	FenceManager(FenceManager&&) = delete;
	FenceManager& operator=(FenceManager&&) = delete;

	static void EraseAll() noexcept;

	static ID3D12Fence& CreateFence(
		const std::uint64_t fenceInitialValue,
		const D3D12_FENCE_FLAGS& flags) noexcept;

private:
	using Fences = tbb::concurrent_unordered_set<ID3D12Fence*>;
	static Fences mFences;

	static std::mutex mMutex;
};

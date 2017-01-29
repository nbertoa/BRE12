#include "FenceManager.h"

#include <memory>

#include <DirectXManager/DirectXManager.h>
#include <Utils/DebugUtils.h>
#include <Utils/NumberGeneration.h>

FenceManager::FenceById FenceManager::mFenceById;
std::mutex FenceManager::mMutex;

std::size_t FenceManager::CreateFence(
	const std::uint64_t fenceInitialValue,
	const D3D12_FENCE_FLAGS& flags,
	ID3D12Fence* &fence) noexcept
{
	mMutex.lock();
	CHECK_HR(DirectXManager::GetDevice().CreateFence(fenceInitialValue, flags, IID_PPV_ARGS(&fence)));
	mMutex.unlock();

	const std::size_t id{ NumberGeneration::GetIncrementalSizeT() };
	FenceById::accessor accessor;
#ifdef _DEBUG
	mFenceById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mFenceById.insert(accessor, id);
	accessor->second = Microsoft::WRL::ComPtr<ID3D12Fence>(fence);
	accessor.release();

	return id;
}

ID3D12Fence& FenceManager::GetFence(const std::size_t id) noexcept {
	FenceById::accessor accessor;
	mFenceById.find(accessor, id);
	ASSERT(!accessor.empty());
	ID3D12Fence* elem{ accessor->second.Get() };
	accessor.release();

	return *elem;
}
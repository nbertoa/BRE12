#include "CommandAllocatorManager.h"

#include <memory>

#include <DirectXManager/DirectXManager.h>
#include <Utils/DebugUtils.h>
#include <Utils/NumberGeneration.h>

namespace {
	std::unique_ptr<CommandAllocatorManager> gManager{ nullptr };
}

CommandAllocatorManager& CommandAllocatorManager::Create() noexcept {
	ASSERT(gManager == nullptr);
	gManager.reset(new CommandAllocatorManager());
	return *gManager.get();
}

CommandAllocatorManager& CommandAllocatorManager::Get() noexcept {
	ASSERT(gManager != nullptr);
	return *gManager.get();
}

std::size_t CommandAllocatorManager::CreateCommandAllocator(
	const D3D12_COMMAND_LIST_TYPE& commandListType, 
	ID3D12CommandAllocator* &commandAllocator) noexcept 
{
	mMutex.lock();
	CHECK_HR(DirectXManager::GetDevice().CreateCommandAllocator(commandListType, IID_PPV_ARGS(&commandAllocator)));
	mMutex.unlock();

	const std::size_t id{ NumberGeneration::GetIncrementalSizeT() };
	CommandAllocatorById::accessor accessor;
#ifdef _DEBUG
	mCommandAllocatorById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mCommandAllocatorById.insert(accessor, id);
	accessor->second = Microsoft::WRL::ComPtr<ID3D12CommandAllocator>(commandAllocator);
	accessor.release();

	return id;
}

ID3D12CommandAllocator& CommandAllocatorManager::GetCommandAllocator(const std::size_t id) noexcept {
	CommandAllocatorById::accessor accessor;
	mCommandAllocatorById.find(accessor, id);
	ASSERT(!accessor.empty());
	ID3D12CommandAllocator* elem{ accessor->second.Get() };
	accessor.release();

	return *elem;
}
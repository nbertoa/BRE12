#include "CommandListManager.h"

#include <memory>

#include <DirectXManager/DirectXManager.h>
#include <Utils/DebugUtils.h>
#include <Utils/NumberGeneration.h>

namespace {
	std::unique_ptr<CommandListManager> gManager{ nullptr };
}

CommandListManager& CommandListManager::Create() noexcept {
	ASSERT(gManager == nullptr);
	gManager.reset(new CommandListManager());
	return *gManager.get();
}

CommandListManager& CommandListManager::Get() noexcept {
	ASSERT(gManager != nullptr);
	return *gManager.get();
}

std::size_t CommandListManager::CreateCommandList(
	const D3D12_COMMAND_LIST_TYPE& commandListType,
	ID3D12CommandAllocator& commandAllocator,
	ID3D12GraphicsCommandList* &commandList) noexcept
{
	mMutex.lock();
	CHECK_HR(DirectXManager::GetDevice().CreateCommandList(0U, commandListType, &commandAllocator, nullptr, IID_PPV_ARGS(&commandList)));
	mMutex.unlock();

	const std::size_t id{ NumberGeneration::GetIncrementalSizeT() };
	CommandListById::accessor accessor;
#ifdef _DEBUG
	mCommandListById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mCommandListById.insert(accessor, id);
	accessor->second = Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>(commandList);
	accessor.release();

	return id;
}

ID3D12GraphicsCommandList& CommandListManager::GetCommandList(const std::size_t id) noexcept {
	CommandListById::accessor accessor;
	mCommandListById.find(accessor, id);
	ASSERT(!accessor.empty());
	ID3D12GraphicsCommandList* elem{ accessor->second.Get() };
	accessor.release();

	return *elem;
}
#include "CommandAllocatorManager.h"

#include <DirectXManager/DirectXManager.h>
#include <Utils/DebugUtils.h>

CommandAllocatorManager::CommandAllocators CommandAllocatorManager::mCommandAllocators;
std::mutex CommandAllocatorManager::mMutex;

void CommandAllocatorManager::EraseAll() noexcept {
	for (ID3D12CommandAllocator* commandAllocator : mCommandAllocators) {
		ASSERT(commandAllocator != nullptr);
		commandAllocator->Release();
	}
}

ID3D12CommandAllocator& CommandAllocatorManager::CreateCommandAllocator(
	const D3D12_COMMAND_LIST_TYPE& commandListType) noexcept 
{
	ID3D12CommandAllocator* commandAllocator{ nullptr };

	mMutex.lock();
	CHECK_HR(DirectXManager::GetDevice().CreateCommandAllocator(commandListType, IID_PPV_ARGS(&commandAllocator)));
	mMutex.unlock();

	ASSERT(commandAllocator != nullptr);
	mCommandAllocators.insert(commandAllocator);

	return *commandAllocator;
}
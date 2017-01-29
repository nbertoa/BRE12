#include "CommandQueueManager.h"

#include <memory>

#include <DirectXManager/DirectXManager.h>
#include <Utils/DebugUtils.h>
#include <Utils/NumberGeneration.h>

CommandQueueManager::CommandQueueById CommandQueueManager::mCommandQueueById;
std::mutex CommandQueueManager::mMutex;

std::size_t CommandQueueManager::CreateCommandQueue(
	const D3D12_COMMAND_QUEUE_DESC& descriptor, 
	ID3D12CommandQueue* &commandQueue) noexcept 
{
	mMutex.lock();
	CHECK_HR(DirectXManager::GetDevice().CreateCommandQueue(&descriptor, IID_PPV_ARGS(&commandQueue)));
	mMutex.unlock();

	const std::size_t id{ NumberGeneration::GetIncrementalSizeT() };
	CommandQueueById::accessor accessor;
#ifdef _DEBUG
	mCommandQueueById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mCommandQueueById.insert(accessor, id);
	accessor->second = Microsoft::WRL::ComPtr<ID3D12CommandQueue>(commandQueue);
	accessor.release();

	return id;
}

ID3D12CommandQueue& CommandQueueManager::GetCommandQueue(const std::size_t id) noexcept {
	CommandQueueById::accessor accessor;
	mCommandQueueById.find(accessor, id);
	ASSERT(!accessor.empty());
	ID3D12CommandQueue* elem{ accessor->second.Get() };
	accessor.release();

	return *elem;
}
#include "CommandQueueManager.h"

#include <DirectXManager/DirectXManager.h>
#include <Utils/DebugUtils.h>

CommandQueueManager::CommandQueues CommandQueueManager::mCommandQueues;
std::mutex CommandQueueManager::mMutex;

CommandQueueManager::~CommandQueueManager() {
	for (ID3D12CommandQueue* commandQueue : mCommandQueues) {
		ASSERT(commandQueue != nullptr);
		commandQueue->Release();
		delete commandQueue;
	}
}

ID3D12CommandQueue& CommandQueueManager::CreateCommandQueue(const D3D12_COMMAND_QUEUE_DESC& descriptor) noexcept {
	ID3D12CommandQueue* commandQueue{ nullptr };

	mMutex.lock();
	CHECK_HR(DirectXManager::GetDevice().CreateCommandQueue(&descriptor, IID_PPV_ARGS(&commandQueue)));
	mMutex.unlock();

	ASSERT(commandQueue != nullptr);
	mCommandQueues.insert(commandQueue);

	return *commandQueue;
}
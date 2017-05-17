#include "CommandQueueManager.h"

#include <DirectXManager/DirectXManager.h>
#include <Utils/DebugUtils.h>

namespace BRE {
tbb::concurrent_unordered_set<ID3D12CommandQueue*> CommandQueueManager::mCommandQueues;
std::mutex CommandQueueManager::mMutex;

void
CommandQueueManager::Clear() noexcept
{
    for (ID3D12CommandQueue* commandQueue : mCommandQueues) {
        BRE_ASSERT(commandQueue != nullptr);
        commandQueue->Release();
    }

    mCommandQueues.clear();
}

ID3D12CommandQueue&
CommandQueueManager::CreateCommandQueue(const D3D12_COMMAND_QUEUE_DESC& descriptor) noexcept
{
    ID3D12CommandQueue* commandQueue{ nullptr };

    mMutex.lock();
    BRE_CHECK_HR(DirectXManager::GetDevice().CreateCommandQueue(&descriptor,
                                                                IID_PPV_ARGS(&commandQueue)));
    mMutex.unlock();

    BRE_ASSERT(commandQueue != nullptr);
    mCommandQueues.insert(commandQueue);

    return *commandQueue;
}
}
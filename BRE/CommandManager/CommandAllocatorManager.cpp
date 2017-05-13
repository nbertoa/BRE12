#include "CommandAllocatorManager.h"

#include <DirectXManager/DirectXManager.h>
#include <Utils/DebugUtils.h>

namespace BRE {
tbb::concurrent_unordered_set<ID3D12CommandAllocator*> CommandAllocatorManager::mCommandAllocators;
std::mutex CommandAllocatorManager::mMutex;

void 
CommandAllocatorManager::Clear() noexcept
{
    for (ID3D12CommandAllocator* commandAllocator : mCommandAllocators) {
        BRE_ASSERT(commandAllocator != nullptr);
        commandAllocator->Release();
    }

    mCommandAllocators.clear();
}

ID3D12CommandAllocator&
CommandAllocatorManager::CreateCommandAllocator(const D3D12_COMMAND_LIST_TYPE& commandListType) noexcept
{
    ID3D12CommandAllocator* commandAllocator{ nullptr };

    mMutex.lock();
    BRE_CHECK_HR(DirectXManager::GetDevice().CreateCommandAllocator(commandListType, IID_PPV_ARGS(&commandAllocator)));
    mMutex.unlock();

    BRE_ASSERT(commandAllocator != nullptr);
    mCommandAllocators.insert(commandAllocator);

    return *commandAllocator;
}
}
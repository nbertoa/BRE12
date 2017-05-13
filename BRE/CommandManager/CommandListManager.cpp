#include "CommandListManager.h"

#include <DirectXManager/DirectXManager.h>
#include <Utils/DebugUtils.h>

namespace BRE {
tbb::concurrent_unordered_set<ID3D12GraphicsCommandList*> CommandListManager::mCommandLists;
std::mutex CommandListManager::mMutex;

void
CommandListManager::Clear() noexcept
{
    for (ID3D12GraphicsCommandList* commandList : mCommandLists) {
        BRE_ASSERT(commandList != nullptr);
        commandList->Release();
    }

    mCommandLists.clear();
}

ID3D12GraphicsCommandList&
CommandListManager::CreateCommandList(const D3D12_COMMAND_LIST_TYPE& commandListType,
                                      ID3D12CommandAllocator& commandAllocator) noexcept
{
    ID3D12GraphicsCommandList* commandList{ nullptr };

    mMutex.lock();
    BRE_CHECK_HR(DirectXManager::GetDevice().CreateCommandList(0U,
                                                               commandListType,
                                                               &commandAllocator,
                                                               nullptr,
                                                               IID_PPV_ARGS(&commandList)));
    mMutex.unlock();

    BRE_ASSERT(commandList != nullptr);
    mCommandLists.insert(commandList);

    return *commandList;
}
}
#include "CommandListManager.h"

#include <DirectXManager/DirectXManager.h>
#include <Utils/DebugUtils.h>

CommandListManager::CommandLists CommandListManager::mCommandLists;
std::mutex CommandListManager::mMutex;

void
CommandListManager::EraseAll() noexcept
{
    for (ID3D12GraphicsCommandList* commandList : mCommandLists) {
        ASSERT(commandList != nullptr);
        commandList->Release();
    }
}

ID3D12GraphicsCommandList&
CommandListManager::CreateCommandList(const D3D12_COMMAND_LIST_TYPE& commandListType,
                                      ID3D12CommandAllocator& commandAllocator) noexcept
{
    ID3D12GraphicsCommandList* commandList{ nullptr };

    mMutex.lock();
    CHECK_HR(DirectXManager::GetDevice().CreateCommandList(0U,
                                                           commandListType,
                                                           &commandAllocator,
                                                           nullptr,
                                                           IID_PPV_ARGS(&commandList)));
    mMutex.unlock();

    ASSERT(commandList != nullptr);
    mCommandLists.insert(commandList);

    return *commandList;
}
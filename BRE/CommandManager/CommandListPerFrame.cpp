#include "CommandListPerFrame.h"

#include <d3d12.h>

#include <CommandManager/CommandAllocatorManager.h>
#include <CommandManager/CommandListManager.h>

namespace {
void BuildCommandObjects(ID3D12GraphicsCommandList* &commandList,
                         ID3D12CommandAllocator* commandAllocators[]) noexcept
{
    ASSERT(commandList == nullptr);

#ifdef _DEBUG
    for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
        ASSERT(commandAllocators[i] == nullptr);
    }
#endif

    for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
        commandAllocators[i] = &CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
    }

    commandList = &CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *commandAllocators[0]);

    // Start off in a closed state.  This is because the first time we refer 
    // to the command list we will Reset it, and it needs to be closed before
    // calling Reset.
    commandList->Close();
}
}

CommandListPerFrame::CommandListPerFrame()
{
    BuildCommandObjects(mCommandList, mCommandAllocators);
}

ID3D12GraphicsCommandList&
CommandListPerFrame::ResetWithNextCommandAllocator(ID3D12PipelineState* pso) noexcept
{
    ID3D12CommandAllocator* commandAllocator{ mCommandAllocators[mCurrentFrameIndex] };
    ASSERT(commandAllocator != nullptr);
    ASSERT(mCommandList != nullptr);

    CHECK_HR(commandAllocator->Reset());
    CHECK_HR(mCommandList->Reset(commandAllocator, pso));

    mCurrentFrameIndex = (mCurrentFrameIndex + 1) % SettingsManager::sQueuedFrameCount;

    return *mCommandList;
}
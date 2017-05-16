#include "CommandListPerFrame.h"

#include <d3d12.h>

#include <CommandManager/CommandAllocatorManager.h>
#include <CommandManager/CommandListManager.h>

namespace BRE {
namespace {
void
BuildCommandObjects(ID3D12GraphicsCommandList* &commandList,
                    ID3D12CommandAllocator* commandAllocators[]) noexcept
{
    BRE_ASSERT(commandList == nullptr);

#ifdef _DEBUG
    for (std::uint32_t i = 0U; i < ApplicationSettings::sQueuedFrameCount; ++i) {
        BRE_ASSERT(commandAllocators[i] == nullptr);
    }
#endif

    for (std::uint32_t i = 0U; i < ApplicationSettings::sQueuedFrameCount; ++i) {
        commandAllocators[i] = 
        &CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
    }

    commandList = 
    &CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *commandAllocators[0]);

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
CommandListPerFrame::ResetCommandListWithNextCommandAllocator(ID3D12PipelineState* pso) noexcept
{
    ID3D12CommandAllocator* commandAllocator{ mCommandAllocators[mCurrentFrameIndex] };
    BRE_ASSERT(commandAllocator != nullptr);
    BRE_ASSERT(mCommandList != nullptr);

    BRE_CHECK_HR(commandAllocator->Reset());
    BRE_CHECK_HR(mCommandList->Reset(commandAllocator, pso));

    mCurrentFrameIndex = (mCurrentFrameIndex + 1) % ApplicationSettings::sQueuedFrameCount;

    return *mCommandList;
}
}
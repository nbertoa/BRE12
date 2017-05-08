#include "CommandListExecutor.h"

#include <memory>

#include <CommandManager\CommandQueueManager.h>
#include <CommandManager\FenceManager.h>

CommandListExecutor* CommandListExecutor::sExecutor{ nullptr };

void
CommandListExecutor::Create(const std::uint32_t maxNumCmdLists) noexcept
{
    ASSERT(sExecutor == nullptr);

    tbb::empty_task* parent{ new (tbb::task::allocate_root()) tbb::empty_task };

    // 1 reference for the parent + 1 reference for the child
    parent->set_ref_count(2);

    sExecutor = new (parent->allocate_child()) CommandListExecutor(maxNumCmdLists);
}

CommandListExecutor&
CommandListExecutor::Get() noexcept
{
    ASSERT(sExecutor != nullptr);

    return *sExecutor;
}

CommandListExecutor::CommandListExecutor(const std::uint32_t maxNumberOfCommandListsToExecute)
    : mMaxNumberOfCommandListsToExecute(maxNumberOfCommandListsToExecute)
{
    ASSERT(maxNumberOfCommandListsToExecute > 0U);

    D3D12_COMMAND_QUEUE_DESC commandQueueDescriptor = {};
    commandQueueDescriptor.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    commandQueueDescriptor.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    mCommandQueue = &CommandQueueManager::CreateCommandQueue(commandQueueDescriptor);
    ASSERT(mCommandQueue != nullptr);

    mFence = &FenceManager::CreateFence(0U, D3D12_FENCE_FLAG_NONE);

    parent()->spawn(*this);
}

tbb::task*
CommandListExecutor::execute()
{
    ASSERT(mMaxNumberOfCommandListsToExecute > 0);

    ID3D12CommandList* *pendingCommandLists{ new ID3D12CommandList*[mMaxNumberOfCommandListsToExecute] };
    while (mTerminate == false) {
        // Pop at most mMaxNumberOfCommandListsToExecute from command list queue
        while (mPendingCommandListCount < mMaxNumberOfCommandListsToExecute &&
               mCommandListsToExecute.try_pop(pendingCommandLists[mPendingCommandListCount])) {
            ++mPendingCommandListCount;
        }

        // Execute command lists (if any)
        if (mPendingCommandListCount != 0U) {
            mCommandQueue->ExecuteCommandLists(mPendingCommandListCount, pendingCommandLists);
            mExecutedCommandListCount += mPendingCommandListCount;
            mPendingCommandListCount = 0U;
        } else {
            Sleep(0U);
        }
    }

    delete[] pendingCommandLists;

    return nullptr;
}

void
CommandListExecutor::SignalFenceAndWaitForCompletion(
    ID3D12Fence& fence,
    const std::uint64_t valueToSignal,
    const std::uint64_t valueToWaitFor) noexcept
{
    const std::uint64_t completedFenceValue = fence.GetCompletedValue();
    CHECK_HR(mCommandQueue->Signal(&fence, valueToSignal));

    // Wait until the GPU has completed commands up to this fence point.
    if (completedFenceValue < valueToWaitFor) {
        const HANDLE eventHandle{ CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS) };
        ASSERT(eventHandle);

        // Fire event when GPU hits current fence.  
        CHECK_HR(fence.SetEventOnCompletion(valueToWaitFor, eventHandle));

        // Wait until the GPU hits current fence event is fired.
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

void
CommandListExecutor::ExecuteCommandListAndWaitForCompletion(ID3D12CommandList& cmdList) noexcept
{
    ASSERT(mCommandQueue != nullptr);
    ASSERT(mFence != nullptr);

    ID3D12CommandList* commandLists[1U]{ &cmdList };
    mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    const std::uint64_t valueToSignal = mFence->GetCompletedValue() + 1UL;
    SignalFenceAndWaitForCompletion(*mFence, valueToSignal, valueToSignal);
}

void
CommandListExecutor::Terminate() noexcept
{
    mTerminate = true;
    parent()->wait_for_all();
}
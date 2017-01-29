#include "CommandListExecutor.h"

#include <memory>

#include <Utils/DebugUtils.h>

CommandListExecutor* CommandListExecutor::sExecutor{ nullptr };

void CommandListExecutor::Create(
	ID3D12CommandQueue& cmdQueue, 
	const std::uint32_t maxNumCmdLists) noexcept 
{
	ASSERT(sExecutor == nullptr);

	tbb::empty_task* parent{ new (tbb::task::allocate_root()) tbb::empty_task };

	// 1 reference for the parent + 1 reference for the child
	parent->set_ref_count(2);

	sExecutor = new (parent->allocate_child()) CommandListExecutor(cmdQueue, maxNumCmdLists);
}

CommandListExecutor& CommandListExecutor::Get() noexcept {
	ASSERT(sExecutor != nullptr);
	return *sExecutor;
}

CommandListExecutor::CommandListExecutor(
	ID3D12CommandQueue& commandQueue, 
	const std::uint32_t maxNumberOfCommandListsToExecute)
	: mMaxNumberOfCommandListsToExecute(maxNumberOfCommandListsToExecute)
	, mCommandQueue(&commandQueue)
{
	ASSERT(maxNumberOfCommandListsToExecute > 0U);
	parent()->spawn(*this);
}

tbb::task* CommandListExecutor::execute() {
	ASSERT(mMaxNumberOfCommandListsToExecute > 0);

	ID3D12CommandList* *pendingCommandLists{ new ID3D12CommandList*[mMaxNumberOfCommandListsToExecute] };
	while (mTerminate == false) {
		// Pop at most mMaxNumberOfCommandListsToExecute from command list queue
		while (mPendingCommandListCount < mMaxNumberOfCommandListsToExecute && 
			   mCommandListsToExecute.try_pop(pendingCommandLists[mPendingCommandListCount])) 
		{
			++mPendingCommandListCount;
		}

		// Execute command lists (if any)
		if (mPendingCommandListCount != 0U) {
			mCommandQueue->ExecuteCommandLists(mPendingCommandListCount, pendingCommandLists);
			mExecutedCommandListCount += mPendingCommandListCount;
			mPendingCommandListCount = 0U;
		}
		else {
			Sleep(0U);
		}
	}

	delete[] pendingCommandLists;

	return nullptr;
}

void CommandListExecutor::Terminate() noexcept {
	mTerminate = true;
	parent()->wait_for_all();
}
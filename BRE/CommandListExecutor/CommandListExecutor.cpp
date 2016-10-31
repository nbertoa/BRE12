#include "CommandListExecutor.h"

#include <Utils/DebugUtils.h>

CommandListExecutor* CommandListExecutor::Create(ID3D12CommandQueue* cmdQueue, const std::uint32_t maxNumCmdLists) noexcept {
	tbb::empty_task* parent{ new (tbb::task::allocate_root()) tbb::empty_task };

	// 1 reference for the parent + 1 reference for the child
	parent->set_ref_count(2);
	return new (parent->allocate_child()) CommandListExecutor(cmdQueue, maxNumCmdLists);
}

CommandListExecutor::CommandListExecutor(ID3D12CommandQueue* cmdQueue, const std::uint32_t maxNumCmdLists)
	: mMaxNumCmdLists(maxNumCmdLists)
	, mCmdQueue(cmdQueue)
{
	ASSERT(maxNumCmdLists > 0U);
	parent()->spawn(*this);
}

tbb::task* CommandListExecutor::execute() {
	ASSERT(mMaxNumCmdLists > 0);

	ID3D12CommandList* *cmdLists{ new ID3D12CommandList*[mMaxNumCmdLists] };
	while (!mTerminate) {
		// Pop at most mMaxNumCmdLists from command list queue
		while (mPendingCmdLists < mMaxNumCmdLists && mCmdListQueue.try_pop(cmdLists[mPendingCmdLists])) {
			++mPendingCmdLists;
		}

		// Execute command lists (if any)
		if (mPendingCmdLists != 0U) {
			mCmdQueue->ExecuteCommandLists(mPendingCmdLists, cmdLists);
			mExecutedCmdLists += mPendingCmdLists;
			mPendingCmdLists = 0U;
		}
		else {
			Sleep(0U);
		}
	}

	delete[] cmdLists;

	return nullptr;
}

void CommandListExecutor::Terminate() noexcept {
	mTerminate = true;
	parent()->wait_for_all();
}
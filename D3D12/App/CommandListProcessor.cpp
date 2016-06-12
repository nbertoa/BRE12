#include "CommandListProcessor.h"

#include <Utils/DebugUtils.h>

tbb::empty_task* CommandListProcessor::Create(CommandListProcessor* &cmdListProcessor, ID3D12CommandQueue* cmdQueue, const std::uint32_t maxNumCmdLists) {
	tbb::empty_task* parent{ new (tbb::task::allocate_root()) tbb::empty_task };
	parent->set_ref_count(2);
	cmdListProcessor = new (parent->allocate_child()) CommandListProcessor(cmdQueue, maxNumCmdLists);
	parent->spawn(*cmdListProcessor);

	return parent;
}

CommandListProcessor::CommandListProcessor(ID3D12CommandQueue* cmdQueue, const std::uint32_t maxNumCmdLists)
	: mCmdQueue(cmdQueue)	
	, mMaxNumCmdLists(maxNumCmdLists)
{
	ASSERT(maxNumCmdLists > 0U);
}

tbb::task* CommandListProcessor::execute() {
	ASSERT(mMaxNumCmdLists > 0);

	ID3D12CommandList* *cmdLists{ new ID3D12CommandList*[mMaxNumCmdLists] };
	std::uint32_t cmdListsCount{ 0U };
	while (mCmdQueue != nullptr) {
		// Pop at most MAX_COMMAND_LISTS from command list queue
		while (cmdListsCount < mMaxNumCmdLists && mCmdListQueue.try_pop(cmdLists[cmdListsCount])) {
			++cmdListsCount;
		}
		if (cmdListsCount != 0U) {
			mCmdQueue->ExecuteCommandLists(cmdListsCount, cmdLists);
			cmdListsCount = 0U;
		}
		else {
			Sleep(0U);
		}
	}

	delete[] cmdLists;

	return nullptr;
}
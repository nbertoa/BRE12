#include "CommandListProcessor.h"

#include <Utils/DebugUtils.h>

namespace {
	// Max number of command list to execute in each ExecuteCommandLists call
	const std::uint32_t MAX_COMMAND_LISTS{ 3U };
}

tbb::empty_task* CommandListProcessor::Create(CommandListProcessor* &cmdListProcessor, Microsoft::WRL::ComPtr<ID3D12CommandQueue>& cmdQueue) {
	tbb::empty_task* parent{ new (tbb::task::allocate_root()) tbb::empty_task };
	parent->set_ref_count(2);
	cmdListProcessor = new (parent->allocate_child()) CommandListProcessor(cmdQueue);
	parent->spawn(*cmdListProcessor);

	return parent;
}

CommandListProcessor::CommandListProcessor(Microsoft::WRL::ComPtr<ID3D12CommandQueue>& cmdQueue)
	: mCmdQueue(cmdQueue)
{

}

tbb::task* CommandListProcessor::execute() {
	ASSERT(mCmdQueue.Get() != nullptr);

	ID3D12CommandList* cmdLists[MAX_COMMAND_LISTS]{ nullptr };
	std::uint32_t cmdListsCount{ 0U };
	while (mCmdQueue.Get() != nullptr) {
		// Pop at most MAX_COMMAND_LISTS from command list queue
		while (cmdListsCount < MAX_COMMAND_LISTS && mCmdListQueue.try_pop(cmdLists[cmdListsCount])) {
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

	return nullptr;
}
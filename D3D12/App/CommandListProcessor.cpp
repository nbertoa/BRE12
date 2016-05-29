#include "CommandListProcessor.h"

#include <Utils/DebugUtils.h>

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

	ID3D12CommandList* cmdList{ nullptr };
	while (mCmdQueue.Get() != nullptr) {
		if (mCmdListQueue.try_pop(cmdList)) {
			ASSERT(cmdList != nullptr);
			mCmdQueue->ExecuteCommandLists(1U, &cmdList);
		}
	}

	return nullptr;
}
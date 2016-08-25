#include "Scene.h"

#include <d3d12.h>

CmdListHelper::CmdListHelper(ID3D12CommandQueue& cmdQueue, ID3D12Fence& fence, std::uint64_t& currentFence, ID3D12GraphicsCommandList& cmdList)
	: mCmdQueue(cmdQueue)
	, mFence(fence)
	, mCurrentFence(currentFence)
	, mCmdList(cmdList)
{

}

void CmdListHelper::ExecuteCmdList() noexcept {
	mCmdList.Close();

	ID3D12CommandList* cmdLists[1U]{ &mCmdList };
	mCmdQueue.ExecuteCommandLists(_countof(cmdLists), cmdLists);

	++mCurrentFence;

	CHECK_HR(mCmdQueue.Signal(&mFence, mCurrentFence));

	// Wait until the GPU has completed commands up to this fence point.
	if (mFence.GetCompletedValue() < mCurrentFence) {
		const HANDLE eventHandle{ CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS) };
		ASSERT(eventHandle);

		// Fire event when GPU hits current fence.  
		CHECK_HR(mFence.SetEventOnCompletion(mCurrentFence, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}
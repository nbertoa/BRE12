#include "CmdBuilderTask.h"

#include <Utils/DebugUtils.h>

CmdBuilderTask::CmdBuilderTask(ID3D12Device* device, const D3D12_VIEWPORT& screenViewport, const D3D12_RECT& scissorRect)
	: mDevice(device)
	, mViewport(screenViewport)
	, mScissorRect(scissorRect)
{
	ASSERT(device != nullptr);
}
void CmdBuilderTask::SwitchCmdListAndAlloc() noexcept {
	ASSERT(mInput.mCmdList1 != nullptr);
	ASSERT(mInput.mCmdAlloc1 != nullptr);
	ASSERT(mInput.mCmdList2 != nullptr);
	ASSERT(mInput.mCmdAlloc2 != nullptr);

	ID3D12GraphicsCommandList* tmpCmdList{ mInput.mCmdList1 };
	ID3D12CommandAllocator* tmpCmdAlloc{ mInput.mCmdAlloc1 };
	mInput.mCmdList1 = mInput.mCmdList2;
	mInput.mCmdList2 = tmpCmdList;
	mInput.mCmdAlloc1 = mInput.mCmdAlloc2;
	mInput.mCmdAlloc2 = tmpCmdAlloc;
}

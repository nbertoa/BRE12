#include "CmdListRecorder.h"

#include <CommandManager/CommandManager.h>
#include <Utils/DebugUtils.h>

CmdListRecorder::CmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue)
	: mDevice(device)
	, mCmdListQueue(cmdListQueue)
{
	BuildCommandObjects();
}

bool CmdListRecorder::ValidateData() const noexcept {
	return
		mCmdList != nullptr &&
		mCmdAlloc != nullptr &&
		mCBVHeap != nullptr &&
		mRootSign != nullptr &&
		mPSO != nullptr &&
		mFrameConstants != nullptr &&
		mObjectConstants != nullptr &&
		mGeometryVec.empty() == false && 
		mGeometryVec.size() == mWorldMatricesByGeomIndex.size();
}

void CmdListRecorder::BuildCommandObjects() noexcept {
	ASSERT(mCmdList == nullptr);
	const std::uint32_t allocCount{ _countof(mCmdAlloc) };

#ifdef _DEBUG
	for (std::uint32_t i = 0U; i < allocCount; ++i) {
		ASSERT(mCmdAlloc[i] == nullptr);
	}
#endif

	for (std::uint32_t i = 0U; i < allocCount; ++i) {
		CommandManager::gManager->CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAlloc[i]);
	}

	CommandManager::gManager->CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *mCmdAlloc[0], mCmdList);

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	mCmdList->Close();
}

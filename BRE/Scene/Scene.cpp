#include "Scene.h"

#include <d3d12.h>

#include <CommandManager\CommandManager.h>
#include <ResourceManager\ResourceManager.h>
#include <Utils\DebugUtils.h>

// cmdQueue is used by derived classes
void Scene::Init(ID3D12CommandQueue& /*cmdQueue*/) noexcept {
	ASSERT(ValidateData() == false);

	CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAlloc);
	CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *mCmdAlloc, mCmdList);
	mCmdList->Close();
	ResourceManager::Get().CreateFence(0U, D3D12_FENCE_FLAG_NONE, mFence);

	ASSERT(ValidateData());
}

void Scene::ExecuteCommandList(ID3D12CommandQueue& cmdQueue) const noexcept {
	ASSERT(ValidateData());

	mCmdList->Close();

	ID3D12CommandList* cmdLists[1U]{ mCmdList };
	cmdQueue.ExecuteCommandLists(_countof(cmdLists), cmdLists);

	const std::uint64_t fenceValue = mFence->GetCompletedValue() + 1UL;

	CHECK_HR(cmdQueue.Signal(mFence, fenceValue));

	// Wait until the GPU has completed commands up to this fence point.
	if (mFence->GetCompletedValue() < fenceValue) {
		const HANDLE eventHandle{ CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS) };
		ASSERT(eventHandle);

		// Fire event when GPU hits current fence.  
		CHECK_HR(mFence->SetEventOnCompletion(fenceValue, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

bool 
Scene::ValidateData() const {
	const bool b =
		mCmdAlloc != nullptr &&
		mCmdList != nullptr &&
		mFence != nullptr;

	return b;
}
#include "Scene.h"

#include <d3d12.h>

#include <CommandManager\CommandManager.h>
#include <ResourceManager\ResourceManager.h>
#include <Utils\DebugUtils.h>

// cmdQueue is used by derived classes
void Scene::Init(ID3D12CommandQueue& /*cmdQueue*/) noexcept {
	ASSERT(IsDataValid() == false);

	CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAlloc);
	CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *mCmdAlloc, mCmdList);
	mCmdList->Close();
	ResourceManager::Get().CreateFence(0U, D3D12_FENCE_FLAG_NONE, mFence);

	ASSERT(IsDataValid());
}

bool 
Scene::IsDataValid() const {
	const bool b =
		mCmdAlloc != nullptr &&
		mCmdList != nullptr &&
		mFence != nullptr;

	return b;
}
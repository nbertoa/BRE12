#include "Scene.h"

#include <d3d12.h>

#include <CommandManager\CommandAllocatorManager.h>
#include <CommandManager\CommandListManager.h>
#include <CommandManager\FenceManager.h>
#include <Utils\DebugUtils.h>

void Scene::Init() noexcept {
	ASSERT(IsDataValid() == false);

	CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocators);
	CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *mCommandAllocators, mCommandList);
	mCommandList->Close();
	FenceManager::CreateFence(0U, D3D12_FENCE_FLAG_NONE, mFence);

	ASSERT(IsDataValid());
}

bool 
Scene::IsDataValid() const {
	const bool b =
		mCommandAllocators != nullptr &&
		mCommandList != nullptr &&
		mFence != nullptr;

	return b;
}
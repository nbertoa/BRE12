#include "Scene.h"

#include <d3d12.h>

#include <CommandManager\CommandAllocatorManager.h>
#include <CommandManager\CommandListManager.h>
#include <Utils\DebugUtils.h>

void Scene::Init() noexcept {
	ASSERT(IsDataValid() == false);

	CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocators);
	CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *mCommandAllocators, mCommandList);
	mCommandList->Close();	

	ASSERT(IsDataValid());
}

bool 
Scene::IsDataValid() const {
	const bool b =
		mCommandAllocators != nullptr &&
		mCommandList != nullptr;

	return b;
}
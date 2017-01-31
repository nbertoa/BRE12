#include "Scene.h"

#include <d3d12.h>

#include <CommandManager\CommandAllocatorManager.h>
#include <CommandManager\CommandListManager.h>
#include <Utils\DebugUtils.h>

void Scene::Init() noexcept {
	ASSERT(IsDataValid() == false);

	mCommandAllocator = &CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
	mCommandList = &CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *mCommandAllocator);
	mCommandList->Close();	

	ASSERT(IsDataValid());
}

bool 
Scene::IsDataValid() const {
	const bool b =
		mCommandAllocator != nullptr &&
		mCommandList != nullptr;

	return b;
}
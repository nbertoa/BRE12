#include "ResourceStateManager.h"

#include <memory>

#include <Utils\DebugUtils.h>

namespace {
	std::unique_ptr<ResourceStateManager> gManager{ nullptr };
}

ResourceStateManager& ResourceStateManager::Create() noexcept {
	ASSERT(gManager == nullptr);
	gManager.reset(new ResourceStateManager());
	return *gManager.get();
}
ResourceStateManager& ResourceStateManager::Get() noexcept {
	ASSERT(gManager != nullptr);
	return *gManager.get();
}

void ResourceStateManager::Add(ID3D12Resource& res, const D3D12_RESOURCE_STATES state) noexcept {
	StateByResource::accessor accessor;
#ifdef _DEBUG
	mStateByResource.find(accessor, &res);
	ASSERT(accessor.empty());
#endif
	mStateByResource.insert(accessor, &res);
	accessor->second = state;
	accessor.release();
}

CD3DX12_RESOURCE_BARRIER ResourceStateManager::TransitionState(ID3D12Resource& res, const D3D12_RESOURCE_STATES state) noexcept {
	StateByResource::accessor accessor;
	mStateByResource.find(accessor, &res);
	ASSERT(accessor.empty() == false);

	const D3D12_RESOURCE_STATES oldState = accessor->second;
	ASSERT(oldState != state);
	accessor->second = state;
	accessor.release();

	return CD3DX12_RESOURCE_BARRIER::Transition(&res, oldState, state);
}

void ResourceStateManager::Remove(ID3D12Resource& res) noexcept {
	StateByResource::accessor accessor;
	mStateByResource.find(accessor, &res);
	ASSERT(accessor.empty() == false);
	mStateByResource.erase(accessor);
	accessor.release();
}
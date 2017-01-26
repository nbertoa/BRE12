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

void ResourceStateManager::AddResource(ID3D12Resource& resource, const D3D12_RESOURCE_STATES initialState) noexcept {
	StateByResource::accessor accessor;
#ifdef _DEBUG
	mStateByResource.find(accessor, &resource);
	ASSERT(accessor.empty());
#endif
	mStateByResource.insert(accessor, &resource);
	accessor->second = initialState;
	accessor.release();
}

CD3DX12_RESOURCE_BARRIER ResourceStateManager::ChangeResourceStateAndGetBarrier(
	ID3D12Resource& resource, 
	const D3D12_RESOURCE_STATES newState) noexcept 
{
	StateByResource::accessor accessor;
	mStateByResource.find(accessor, &resource);
	ASSERT(accessor.empty() == false);

	const D3D12_RESOURCE_STATES oldState = accessor->second;
	ASSERT(oldState != newState);
	accessor->second = newState;
	accessor.release();

	return CD3DX12_RESOURCE_BARRIER::Transition(&resource, oldState, newState);
}

void ResourceStateManager::RemoveResource(ID3D12Resource& resource) noexcept {
	StateByResource::accessor accessor;
	mStateByResource.find(accessor, &resource);
	ASSERT(accessor.empty() == false);
	mStateByResource.erase(accessor);
	accessor.release();
}
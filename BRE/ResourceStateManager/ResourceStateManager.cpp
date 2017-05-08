#include "ResourceStateManager.h"

#include <memory>

#include <Utils\DebugUtils.h>

ResourceStateManager::StateByResource ResourceStateManager::mStateByResource;

void 
ResourceStateManager::AddResource(ID3D12Resource& resource, const D3D12_RESOURCE_STATES initialState) noexcept {
	StateByResource::accessor accessor;
#ifdef _DEBUG
	mStateByResource.find(accessor, &resource);
	ASSERT(accessor.empty());
#endif
	mStateByResource.insert(accessor, &resource);
	accessor->second = initialState;
	accessor.release();
}

CD3DX12_RESOURCE_BARRIER 
ResourceStateManager::ChangeResourceStateAndGetBarrier(
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

D3D12_RESOURCE_STATES 
ResourceStateManager::GetResourceState(ID3D12Resource& resource) noexcept {
	StateByResource::accessor accessor;
	mStateByResource.find(accessor, &resource);
	ASSERT(accessor.empty() == false);
	return accessor->second;
}
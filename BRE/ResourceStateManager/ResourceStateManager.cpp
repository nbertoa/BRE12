#include "ResourceStateManager.h"

#include <memory>

#include <Utils\DebugUtils.h>

namespace BRE {
tbb::concurrent_hash_map<ID3D12Resource*, D3D12_RESOURCE_STATES> ResourceStateManager::mStateByResource;
tbb::concurrent_hash_map<ID3D12Resource*, tbb::concurrent_vector<D3D12_RESOURCE_STATES>> ResourceStateManager::mStateByResourceIndexByResource;

void
ResourceStateManager::AddFullResourceTracking(ID3D12Resource& resource,
                                              const D3D12_RESOURCE_STATES initialState) noexcept
{
    tbb::concurrent_hash_map<ID3D12Resource*, D3D12_RESOURCE_STATES>::accessor accessor;
#ifdef _DEBUG
    tbb::concurrent_hash_map<ID3D12Resource*, tbb::concurrent_vector<D3D12_RESOURCE_STATES>>::accessor subresourceAccessor;
    mStateByResourceIndexByResource.find(subresourceAccessor, &resource);
    BRE_ASSERT(subresourceAccessor.empty());

    mStateByResource.find(accessor, &resource);
    BRE_ASSERT(accessor.empty());
#endif
    mStateByResource.insert(accessor, &resource);
    accessor->second = initialState;
    accessor.release();
}

void 
ResourceStateManager::AddSubresourceTracking(ID3D12Resource& resource,
                                             const D3D12_RESOURCE_STATES initialState) noexcept
{
    tbb::concurrent_hash_map<ID3D12Resource*, tbb::concurrent_vector<D3D12_RESOURCE_STATES>>::accessor accessor;
#ifdef _DEBUG
    tbb::concurrent_hash_map<ID3D12Resource*, D3D12_RESOURCE_STATES>::accessor fullResourceAccessor;
    mStateByResource.find(fullResourceAccessor, &resource);
    BRE_ASSERT(fullResourceAccessor.empty());

    mStateByResourceIndexByResource.find(accessor, &resource);
    BRE_ASSERT(accessor.empty());
#endif

    // Get the number of subresources and initialize all the states for the resource to be added
    D3D12_RESOURCE_DESC resourceDesc = resource.GetDesc();
    const std::uint32_t numSubResources = resourceDesc.DepthOrArraySize * resourceDesc.MipLevels;

    mStateByResourceIndexByResource.insert(accessor, &resource);
    for (std::uint32_t i = 0U; i < numSubResources; ++i) {
        accessor->second.push_back(initialState);
    }
    
    accessor.release();
}

CD3DX12_RESOURCE_BARRIER
ResourceStateManager::ChangeResourceStateAndGetBarrier(ID3D12Resource& resource,
                                                       const D3D12_RESOURCE_STATES newState) noexcept
{
    tbb::concurrent_hash_map<ID3D12Resource*, D3D12_RESOURCE_STATES>::accessor accessor;
    mStateByResource.find(accessor, &resource);
    BRE_ASSERT(accessor.empty() == false);

    const D3D12_RESOURCE_STATES oldState = accessor->second;
    BRE_ASSERT(oldState != newState);
    accessor->second = newState;
    accessor.release();

    return CD3DX12_RESOURCE_BARRIER::Transition(&resource, 
                                                oldState, 
                                                newState);
}

CD3DX12_RESOURCE_BARRIER
ResourceStateManager::ChangeSubresourceStateAndGetBarrier(ID3D12Resource& resource,
                                                          const std::uint32_t subresourceIndex,
                                                          const D3D12_RESOURCE_STATES newState) noexcept
{
    tbb::concurrent_hash_map<ID3D12Resource*, tbb::concurrent_vector<D3D12_RESOURCE_STATES>>::accessor accessor;
    mStateByResourceIndexByResource.find(accessor, &resource);
    BRE_ASSERT(accessor.empty() == false);

    tbb::concurrent_vector<D3D12_RESOURCE_STATES>& stateBySubresourceIndex = accessor->second;
    BRE_ASSERT(subresourceIndex < static_cast<std::uint32_t>(stateBySubresourceIndex.size()));
    const D3D12_RESOURCE_STATES oldState = stateBySubresourceIndex[subresourceIndex];
    BRE_ASSERT(oldState != newState);
    stateBySubresourceIndex[subresourceIndex] = newState;
    accessor.release();

    return CD3DX12_RESOURCE_BARRIER::Transition(&resource, 
                                                oldState, 
                                                newState,
                                                subresourceIndex);
}

D3D12_RESOURCE_STATES
ResourceStateManager::GetResourceState(ID3D12Resource& resource) noexcept
{
    tbb::concurrent_hash_map<ID3D12Resource*, D3D12_RESOURCE_STATES>::accessor accessor;
    mStateByResource.find(accessor, &resource);
    BRE_ASSERT(accessor.empty() == false);
    return accessor->second;
}

D3D12_RESOURCE_STATES
ResourceStateManager::GetSubresourceState(ID3D12Resource& resource,
                                          const std::uint32_t subresourceIndex) noexcept
{
    tbb::concurrent_hash_map<ID3D12Resource*, tbb::concurrent_vector<D3D12_RESOURCE_STATES>>::accessor accessor;
    mStateByResourceIndexByResource.find(accessor, &resource);
    BRE_ASSERT(accessor.empty() == false);

    tbb::concurrent_vector<D3D12_RESOURCE_STATES>& stateBySubresourceIndex = accessor->second;
    BRE_ASSERT(subresourceIndex < static_cast<std::uint32_t>(stateBySubresourceIndex.size()));

    return stateBySubresourceIndex[subresourceIndex];
}
}
#pragma once

#include <d3d12.h>
#include <tbb/concurrent_hash_map.h>

#include <DXUtils\d3dx12.h>

struct ID3D12Resource;

namespace BRE {
// To track resource states.
// Its functionality includes:
// - GetResource state registration
// - GetResource state change
// - GetResource unregistration
class ResourceStateManager {
public:
    ResourceStateManager() = delete;
    ~ResourceStateManager() = delete;
    ResourceStateManager(const ResourceStateManager&) = delete;
    const ResourceStateManager& operator=(const ResourceStateManager&) = delete;
    ResourceStateManager(ResourceStateManager&&) = delete;
    ResourceStateManager& operator=(ResourceStateManager&&) = delete;

    // Preconditions:
    // - Resource must not have been registered
    static void AddResource(ID3D12Resource& resource,
                            const D3D12_RESOURCE_STATES initialState) noexcept;

    // Preconditions:
    // - Resource must have been registered
    // - New state must be different than current state
    static CD3DX12_RESOURCE_BARRIER ChangeResourceStateAndGetBarrier(ID3D12Resource& resource,
                                                                     const D3D12_RESOURCE_STATES newState) noexcept;

    // Preconditions:
    // - Resource must have been registered
    static D3D12_RESOURCE_STATES GetResourceState(ID3D12Resource& resource) noexcept;

private:
    using StateByResource = tbb::concurrent_hash_map<ID3D12Resource*, D3D12_RESOURCE_STATES>;
    static StateByResource mStateByResource;
};
}


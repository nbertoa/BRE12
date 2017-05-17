#pragma once

#include <d3d12.h>
#include <tbb/concurrent_hash_map.h>

#include <DXUtils\d3dx12.h>

namespace BRE {
///
/// @brief Responsible to track resource states.
///
/// Its functionality includes:
/// - Resource state registration
/// - Resource state change
/// - Resource unregistration
///
class ResourceStateManager {
public:
    ResourceStateManager() = delete;
    ~ResourceStateManager() = delete;
    ResourceStateManager(const ResourceStateManager&) = delete;
    const ResourceStateManager& operator=(const ResourceStateManager&) = delete;
    ResourceStateManager(ResourceStateManager&&) = delete;
    ResourceStateManager& operator=(ResourceStateManager&&) = delete;

    ///
    /// @brief Add resource
    ///
    /// Resource must not have been registered
    ///
    /// @param resource Resource to add
    /// @param initialState Initial state of the resource
    ///
    static void AddResource(ID3D12Resource& resource,
                            const D3D12_RESOURCE_STATES initialState) noexcept;

    ///
    /// @brief Change resource state and get barrier
    ///
    /// Resource must have been registered. New state must be different than current state.
    ///
    /// @param resource Resource to change state
    /// @param newState New resource state. It must be different than current state.
    ///
    static CD3DX12_RESOURCE_BARRIER ChangeResourceStateAndGetBarrier(ID3D12Resource& resource,
                                                                     const D3D12_RESOURCE_STATES newState) noexcept;

    ///
    /// @brief Get resource state
    /// @param resource Resource to get state. It must have been registered.
    /// @return Resource state
    ///
    static D3D12_RESOURCE_STATES GetResourceState(ID3D12Resource& resource) noexcept;

private:
    static tbb::concurrent_hash_map<ID3D12Resource*, D3D12_RESOURCE_STATES> mStateByResource;
};
}
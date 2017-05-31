#pragma once

#include <d3d12.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_vector.h>

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
    /// @brief Add full resource tracking.
    ///
    /// Resource must not have been registered. You can only change the state of
    /// the entire resource, not a subresource. This is done for performance reasons.
    ///
    /// @param resource Resource to add. It must not been added before (full resource or subresource tracking)
    /// @param initialState Initial state of the resource
    ///
    static void AddFullResourceTracking(ID3D12Resource& resource,
                                        const D3D12_RESOURCE_STATES initialState) noexcept;

    ///
    /// @brief Add subresource tracking.
    ///
    /// Resource must not have been registered. You can only change the state of a single subresource at a time.
    /// This done for performance reasons.
    ///
    /// @param resource Resource to add. It must not been added before (full resource or subresource tracking)
    /// @param initialState Initial state of the resource (this will set the initial state of all its subresources)
    ///
    static void AddSubresourceTracking(ID3D12Resource& resource,
                                       const D3D12_RESOURCE_STATES initialState) noexcept;

    ///
    /// @brief Change resource state and get barrier
    ///
    /// Resource must have been registered. New state must be different than current state.
    ///
    /// @param resource Resource to change state
    /// @param newState New resource state. It must be different than current state.
    /// @return Transition resource barrier
    ///
    static CD3DX12_RESOURCE_BARRIER ChangeResourceStateAndGetBarrier(ID3D12Resource& resource,
                                                                     const D3D12_RESOURCE_STATES newState) noexcept;

    ///
    /// @brief Change subresource state and get barrier
    ///
    /// Resource must have been registered. New state must be different than current state.
    ///
    /// @param resource Resource to change state
    /// @param subresourceIndex Subresource index to change state
    /// @param newState New resource state. It must be different than current state.
    /// @return Transition resource barrier
    ///
    static CD3DX12_RESOURCE_BARRIER ChangeSubresourceStateAndGetBarrier(ID3D12Resource& resource,
                                                                        const std::uint32_t subresourceIndex,
                                                                        const D3D12_RESOURCE_STATES newState) noexcept;

    ///
    /// @brief Get resource state
    /// @param resource Resource to get state. It must have been registered. 
    /// It must have been registered with AddFullResourceTracking.
    /// @return Resource state
    ///
    static D3D12_RESOURCE_STATES GetResourceState(ID3D12Resource& resource) noexcept;

    ///
    /// @brief Get subresource state
    /// @param resource Resource to get state. It must have been registered with AddSubresourceTracking.
    /// @return Subresource state
    ///
    static D3D12_RESOURCE_STATES GetSubresourceState(ID3D12Resource& resource,
                                                     const std::uint32_t subresourceIndex) noexcept;

private:
    static tbb::concurrent_hash_map<ID3D12Resource*, D3D12_RESOURCE_STATES> mStateByResource;
    static tbb::concurrent_hash_map<ID3D12Resource*, tbb::concurrent_vector<D3D12_RESOURCE_STATES>> mStateByResourceIndexByResource;
};
}
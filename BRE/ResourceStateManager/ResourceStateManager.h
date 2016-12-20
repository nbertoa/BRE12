#pragma once

#include <d3d12.h>
#include <tbb/concurrent_hash_map.h>

#include <DXUtils\d3dx12.h>

struct ID3D12Resource;

// Class responsible of tracking resource states to be 
// able to transition between them easier.
// Its functionality includes:
// - Resource state registration
// - Resource state change
// - Resource unregistration
class ResourceStateManager {
public:
	static ResourceStateManager& Create() noexcept;
	static ResourceStateManager& Get() noexcept;

	~ResourceStateManager() = default;
	ResourceStateManager(const ResourceStateManager&) = delete;
	const ResourceStateManager& operator=(const ResourceStateManager&) = delete;
	ResourceStateManager(ResourceStateManager&&) = delete;
	ResourceStateManager& operator=(ResourceStateManager&&) = delete;

	// Register a resource with its initial state. Preconditions:
	// - Resource must not have been registered
	void Add(ID3D12Resource& res, const D3D12_RESOURCE_STATES state) noexcept;

	// Change state of a resource and returns a CD3DX12_RESOURCE_BARRIER. Preconditions:
	// - Resource must have been registered
	// - New state must be different than current state
	CD3DX12_RESOURCE_BARRIER TransitionState(ID3D12Resource& res, const D3D12_RESOURCE_STATES state) noexcept;

	// Unregister a resource. Preconditions:
	// - Resource must have been registered.
	void Remove(ID3D12Resource& res) noexcept;

	void Clear() noexcept { mStateByResource.clear(); }

private:
	ResourceStateManager() = default;

	using StateByResource = tbb::concurrent_hash_map<ID3D12Resource*, D3D12_RESOURCE_STATES>;
	StateByResource mStateByResource;
};
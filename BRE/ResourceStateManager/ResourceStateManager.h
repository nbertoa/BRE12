#pragma once

#include <d3d12.h>
#include <tbb/concurrent_hash_map.h>

#include <DXUtils\d3dx12.h>

struct ID3D12Resource;

// To track resource states.
// Its functionality includes:
// - GetResource state registration
// - GetResource state change
// - GetResource unregistration
class ResourceStateManager {
public:
	// Preconditions:
	// - Create() must be called once
	static ResourceStateManager& Create() noexcept;

	// Preconditions:
	// - Create() must be called before this method
	static ResourceStateManager& Get() noexcept;

	~ResourceStateManager() = default;
	ResourceStateManager(const ResourceStateManager&) = delete;
	const ResourceStateManager& operator=(const ResourceStateManager&) = delete;
	ResourceStateManager(ResourceStateManager&&) = delete;
	ResourceStateManager& operator=(ResourceStateManager&&) = delete;

	// Preconditions:
	// - GetResource must not have been registered
	void AddResource(ID3D12Resource& resource, const D3D12_RESOURCE_STATES initialState) noexcept;
 
	// Preconditions:
	// - GetResource must have been registered
	// - New state must be different than current state
	CD3DX12_RESOURCE_BARRIER ChangeResourceStateAndGetBarrier(
		ID3D12Resource& resource, 
		const D3D12_RESOURCE_STATES newState) noexcept;

	// Preconditions:
	// - GetResource must have been registered.
	void RemoveResource(ID3D12Resource& res) noexcept;

	void Clear() noexcept { mStateByResource.clear(); }

private:
	ResourceStateManager() = default;

	using StateByResource = tbb::concurrent_hash_map<ID3D12Resource*, D3D12_RESOURCE_STATES>;
	StateByResource mStateByResource;
};
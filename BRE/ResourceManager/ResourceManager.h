#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb/concurrent_hash_map.h>
#include <wrl.h>

#include <ResourceManager/UploadBuffer.h>

// This class is responsible to create/get:
// - Textures
// - Buffers
// - Resources
class ResourceManager {
public:
	// Preconditions:
	// - Create() must be called once
	static ResourceManager& Create() noexcept;

	// Preconditions:
	// - Create() must be called before this method
	static ResourceManager& Get() noexcept;
	
	~ResourceManager() = default;
	ResourceManager(const ResourceManager&) = delete;
	const ResourceManager& operator=(const ResourceManager&) = delete;
	ResourceManager(ResourceManager&&) = delete;
	ResourceManager& operator=(ResourceManager&&) = delete;

	std::size_t LoadTextureFromFile(
		const char* textureFilename, 
		ID3D12GraphicsCommandList& commandList,
		ID3D12Resource* &resource, 
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept;

	// Note: uploadBuffer has to be kept alive after the above function calls because
	// the command list has not been executed yet that performs the actual copy.
	// The caller can Release the uploadBuffer after it knows the copy has been executed.
	
	// Preconditions:
	// - "sourceData" must not be nullptr
	// - "sourceDataSize" must be greater than zero
	std::size_t CreateDefaultBuffer(	
		ID3D12GraphicsCommandList& commandList,
		const void* sourceData,
		const std::size_t sourceDataSize,
		ID3D12Resource* &buffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept;

	std::size_t CreateCommittedResource(
		const D3D12_HEAP_PROPERTIES& heapProperties,
		const D3D12_HEAP_FLAGS& heapFlags,
		const D3D12_RESOURCE_DESC& resourceDescriptor,
		const D3D12_RESOURCE_STATES& resourceStates,
		const D3D12_CLEAR_VALUE* clearValue,
		ID3D12Resource* &resource) noexcept;

	// Preconditions:
	// - "id" must be valid.
	ID3D12Resource& GetResource(const std::size_t id) noexcept;

private:
	ResourceManager() = default;

	using ResourceById = tbb::concurrent_hash_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12Resource>>;
	ResourceById mResourceById;

	std::mutex mMutex;
};

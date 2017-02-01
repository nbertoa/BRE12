#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb/concurrent_unordered_set.h>

#include <ResourceManager/UploadBuffer.h>

// This class is responsible to create/get:
// - Textures
// - Buffers
// - Resources
class ResourceManager {
public:
	ResourceManager() = delete;
	~ResourceManager();
	ResourceManager(const ResourceManager&) = delete;
	const ResourceManager& operator=(const ResourceManager&) = delete;
	ResourceManager(ResourceManager&&) = delete;
	ResourceManager& operator=(ResourceManager&&) = delete;

	// If resourceName is nullptr, then it will have 
	// the default name.
	static ID3D12Resource& LoadTextureFromFile(
		const char* textureFilename, 
		ID3D12GraphicsCommandList& commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer,
		const wchar_t* resourceName) noexcept;

	// Note: uploadBuffer has to be kept alive after the above function calls because
	// the command list has not been executed yet that performs the actual copy.
	// The caller can Release the uploadBuffer after it knows the copy has been executed.
	
	// If resourceName is nullptr, then it will have 
	// the default name.
	// Preconditions:
	// - "sourceData" must not be nullptr
	// - "sourceDataSize" must be greater than zero
	static ID3D12Resource& CreateDefaultBuffer(
		ID3D12GraphicsCommandList& commandList,
		const void* sourceData,
		const std::size_t sourceDataSize,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer,
		const wchar_t* resourceName) noexcept;

	// If resourceName is nullptr, then it will have 
	// the default name.
	static ID3D12Resource& CreateCommittedResource(
		const D3D12_HEAP_PROPERTIES& heapProperties,
		const D3D12_HEAP_FLAGS& heapFlags,
		const D3D12_RESOURCE_DESC& resourceDescriptor,
		const D3D12_RESOURCE_STATES& resourceStates,
		const D3D12_CLEAR_VALUE* clearValue,
		const wchar_t* resourceName) noexcept;

private:
	using Resources = tbb::concurrent_unordered_set<ID3D12Resource*>;
	static Resources mResources;

	static std::mutex mMutex;
};

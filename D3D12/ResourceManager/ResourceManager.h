#pragma once

#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <tbb/concurrent_hash_map.h>
#include <wrl.h>

#include <ResourceManager/UploadBuffer.h>
#include <Utils/RandomNumberGenerator.h>

class ResourceManager {
public:
	static std::unique_ptr<ResourceManager> gManager;

	explicit ResourceManager(ID3D12Device& device) : mDevice(device) {}
	ResourceManager(const ResourceManager&) = delete;
	const ResourceManager& operator=(const ResourceManager&) = delete;

	// Returns resource id.
	// Note: uploadBuffer has to be kept alive after the above function calls because
	// the command list has not been executed yet that performs the actual copy.
	// The caller can Release the uploadBuffer after it knows the copy has been executed.
	//
	// Asserts if resource with the same name was already registered
	std::size_t CreateDefaultBuffer(	
		ID3D12GraphicsCommandList& cmdList,
		const void* initData,
		const std::size_t byteSize,
		ID3D12Resource* &defaultBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept;

	// Asserts if resource with the same name was already registered
	std::size_t CreateUploadBuffer(const std::size_t elemSize, const std::uint32_t elemCount, UploadBuffer*& buffer) noexcept;

	// Asserts if resource id is not present
	ID3D12Resource& GetResource(const std::size_t id) noexcept;
	UploadBuffer& GetUploadBuffer(const size_t id) noexcept;

	// This will invalidate all ids.
	__forceinline void ClearDefaultBuffers() noexcept { mResourceById.clear(); }
	__forceinline void ClearUploadBuffers() noexcept { mUploadBufferById.clear(); }
	__forceinline void Clear() noexcept { ClearDefaultBuffers(); ClearUploadBuffers(); }

private:
	ID3D12Device& mDevice;

	using ResourceById = tbb::concurrent_hash_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12Resource>>;
	ResourceById mResourceById;

	using UploadBufferById = tbb::concurrent_hash_map<std::size_t, std::unique_ptr<UploadBuffer>>;
	UploadBufferById mUploadBufferById;

	RandomNumberGenerator mRandGen;
};

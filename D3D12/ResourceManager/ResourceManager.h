#pragma once

#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <tbb/concurrent_hash_map.h>
#include <tbb/mutex.h>
#include <wrl.h>

#include <ResourceManager/UploadBuffer.h>

class ResourceManager {
public:
	static std::unique_ptr<ResourceManager> gManager;

	explicit ResourceManager(ID3D12Device& device) : mDevice(device) {}
	ResourceManager(const ResourceManager&) = delete;
	const ResourceManager& operator=(const ResourceManager&) = delete;

	// Note: uploadBuffer has to be kept alive after the above function calls because
	// the command list has not been executed yet that performs the actual copy.
	// The caller can Release the uploadBuffer after it knows the copy has been executed.
	std::size_t CreateDefaultBuffer(	
		ID3D12GraphicsCommandList& cmdList,
		const void* initData,
		const std::size_t byteSize,
		ID3D12Resource* &defaultBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept;

	std::size_t CreateCommittedResource(
		const D3D12_HEAP_PROPERTIES& heapProps,
		const D3D12_HEAP_FLAGS& heapFlags,
		const D3D12_RESOURCE_DESC& resDesc,
		const D3D12_RESOURCE_STATES& resStates,
		const D3D12_CLEAR_VALUE& clearValue,
		ID3D12Resource* &res) noexcept;

	std::size_t CreateUploadBuffer(const std::size_t elemSize, const std::uint32_t elemCount, UploadBuffer*& buffer) noexcept;
	std::size_t CreateDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_DESC& desc, ID3D12DescriptorHeap* &descHeap) noexcept;
	std::size_t CreateFence(const std::uint64_t initValue, const D3D12_FENCE_FLAGS& flags, ID3D12Fence* &fence) noexcept;

	// Asserts if resource id is not present
	ID3D12Resource& GetResource(const std::size_t id) noexcept;
	UploadBuffer& GetUploadBuffer(const std::size_t id) noexcept;
	ID3D12DescriptorHeap& GetDescriptorHeap(const std::size_t id) noexcept;
	ID3D12Fence& GetFence(const std::size_t id) noexcept;

	void EraseResource(const std::size_t id) noexcept;
	void EraseUploadBuffer(const std::size_t id) noexcept;
	void EraseDescHeap(const std::size_t id) noexcept;
	void EraseFence(const std::size_t id) noexcept;

	// This will invalidate all ids.
	__forceinline void ClearResources() noexcept { mResourceById.clear(); }
	__forceinline void ClearUploadBuffers() noexcept { mUploadBufferById.clear(); }
	__forceinline void ClearDescriptorHeaps() noexcept { mDescHeapById.clear(); }
	__forceinline void ClearFences() noexcept { mFenceById.clear(); }
	__forceinline void Clear() noexcept { ClearResources(); ClearUploadBuffers(); ClearDescriptorHeaps(); ClearFences(); }

private:
	ID3D12Device& mDevice;

	using ResourceById = tbb::concurrent_hash_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12Resource>>;
	ResourceById mResourceById;

	using UploadBufferById = tbb::concurrent_hash_map<std::size_t, std::unique_ptr<UploadBuffer>>;
	UploadBufferById mUploadBufferById;

	using DescHeapById = tbb::concurrent_hash_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>>;
	DescHeapById mDescHeapById;

	using FenceById = tbb::concurrent_hash_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12Fence>>;
	FenceById mFenceById;

	tbb::mutex mMutex;
};

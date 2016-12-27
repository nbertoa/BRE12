#include "ResourceManager.h"

#include <memory>

#include <DXUtils/D3DFactory.h>
#include <DXUtils/d3dx12.h>
#include <ResourceManager\DDSTextureLoader.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <SettingsManager\SettingsManager.h>
#include <Utils/DebugUtils.h>
#include <Utils/NumberGeneration.h>
#include <Utils\StringUtils.h>

namespace {
	std::unique_ptr<ResourceManager> gManager{ nullptr };
}

ResourceManager& ResourceManager::Create(ID3D12Device& device) noexcept {
	ASSERT(gManager == nullptr);
	gManager.reset(new ResourceManager(device));
	return *gManager.get();
}
ResourceManager& ResourceManager::Get() noexcept {
	ASSERT(gManager != nullptr);
	return *gManager.get();
}

ResourceManager::ResourceManager(ID3D12Device& device)
	: mDevice(device)
{
}

std::size_t ResourceManager::LoadTextureFromFile(
	const char* filename, 
	ID3D12Resource* &res, 
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer,
	ID3D12GraphicsCommandList& cmdList) noexcept {
	ASSERT(filename != nullptr);
	std::string filePath(SettingsManager::sResourcesPath);
	filePath += filename;

	const std::wstring filePathW(StringUtils::ToWideString(filePath));

	Microsoft::WRL::ComPtr<ID3D12Resource> resource;

	mMutex.lock();
	CHECK_HR(DirectX::CreateDDSTextureFromFile12(&mDevice, &cmdList, filePathW.c_str(), resource, uploadBuffer));
	mMutex.unlock();

	const std::size_t id{ NumberGeneration::IncrementalSizeT() };
	ResourceById::accessor accessor;
#ifdef _DEBUG
	mResourceById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mResourceById.insert(accessor, id);
	accessor->second = resource;
	accessor.release();

	res = resource.Get();

	return id;
}

std::size_t ResourceManager::CreateDefaultBuffer(
	ID3D12GraphicsCommandList& cmdList,
	const void* initData,
	const std::size_t byteSize,
	ID3D12Resource* &defaultBuffer,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept
{
	ASSERT(initData != nullptr);
	ASSERT(byteSize > 0);
	
	// Create the actual default buffer resource.
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1U;
	heapProps.VisibleNodeMask = 1U;

	CD3DX12_RESOURCE_DESC resDesc{ CD3DX12_RESOURCE_DESC::Buffer(byteSize) };
	mMutex.lock();
	CHECK_HR(mDevice.CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&defaultBuffer)));

	// In order to copy CPU memory data into our default buffer, we need to create
	// an intermediate upload heap. 
	heapProps = D3D12_HEAP_PROPERTIES{};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1U;
	heapProps.VisibleNodeMask = 1U;
	resDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

	CHECK_HR(mDevice.CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf())));
	mMutex.unlock();

	// Describe the data we want to copy into the default buffer.
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	// Schedule to copy the data to the default buffer resource. At a high level, the helper function UpdateSubresources
	// will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
	// the intermediate upload heap data will be copied to mBuffer.
	CD3DX12_RESOURCE_BARRIER resBarrier{ CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST) };
	cmdList.ResourceBarrier(1, &resBarrier);
	UpdateSubresources<1>(&cmdList, defaultBuffer, uploadBuffer.Get(), 0, 0, 1, &subResourceData);
	resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	cmdList.ResourceBarrier(1, &resBarrier);

	const std::size_t id{ NumberGeneration::IncrementalSizeT() };
	ResourceById::accessor accessor;
#ifdef _DEBUG
	mResourceById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mResourceById.insert(accessor, id);
	accessor->second = Microsoft::WRL::ComPtr<ID3D12Resource>(defaultBuffer);
	accessor.release();

	return id;
}

std::size_t ResourceManager::CreateCommittedResource(
	const D3D12_HEAP_PROPERTIES& heapProps,
	const D3D12_HEAP_FLAGS& heapFlags,
	const D3D12_RESOURCE_DESC& resDesc,
	const D3D12_RESOURCE_STATES& resStates,
	const D3D12_CLEAR_VALUE* clearValue,
	ID3D12Resource* &res) noexcept
{
	mMutex.lock();
	CHECK_HR(mDevice.CreateCommittedResource(&heapProps, heapFlags, &resDesc, resStates, clearValue, IID_PPV_ARGS(&res)));
	mMutex.unlock();

	const std::size_t id{ NumberGeneration::IncrementalSizeT() };
	ResourceById::accessor accessor;
#ifdef _DEBUG
	mResourceById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mResourceById.insert(accessor, id);
	accessor->second = Microsoft::WRL::ComPtr<ID3D12Resource>(res);
	accessor.release();

	ResourceStateManager::Get().Add(*res, resStates);

	return id;
}

std::size_t ResourceManager::CreateFence(const std::uint64_t initValue, const D3D12_FENCE_FLAGS& flags, ID3D12Fence* &fence) noexcept {
	mMutex.lock();
	CHECK_HR(mDevice.CreateFence(initValue, flags, IID_PPV_ARGS(&fence)));
	mMutex.unlock();

	const std::size_t id{ NumberGeneration::IncrementalSizeT() };
	FenceById::accessor accessor;
#ifdef _DEBUG
	mFenceById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mFenceById.insert(accessor, id);
	accessor->second = Microsoft::WRL::ComPtr<ID3D12Fence>(fence);
	accessor.release();

	return id;
}

std::size_t ResourceManager::CreateUploadBuffer(const std::size_t elemSize, const std::uint32_t elemCount, UploadBuffer*& buffer) noexcept {
	const std::size_t id{ NumberGeneration::IncrementalSizeT() };
	UploadBufferById::accessor accessor;
#ifdef _DEBUG
	mUploadBufferById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mUploadBufferById.insert(accessor, id);
	accessor->second = std::make_unique<UploadBuffer>(mDevice, elemSize, elemCount);
	buffer = accessor->second.get();
	accessor.release();

	return id;
}

ID3D12Resource& ResourceManager::GetResource(const std::size_t id) noexcept {
	ResourceById::accessor accessor;
	mResourceById.find(accessor, id);
	ASSERT(!accessor.empty());
	ID3D12Resource* elem{ accessor->second.Get() };
	accessor.release();

	return *elem;
}

UploadBuffer& ResourceManager::GetUploadBuffer(const size_t id) noexcept {
	UploadBufferById::accessor accessor;
	mUploadBufferById.find(accessor, id);
	ASSERT(!accessor.empty());
	UploadBuffer* elem{ accessor->second.get() };
	accessor.release();

	return *elem;
}

ID3D12Fence& ResourceManager::GetFence(const std::size_t id) noexcept {
	FenceById::accessor accessor;
	mFenceById.find(accessor, id);
	ASSERT(!accessor.empty());
	ID3D12Fence* elem{ accessor->second.Get() };
	accessor.release();

	return *elem;
}

void ResourceManager::EraseResource(const std::size_t id) noexcept {
	ResourceById::accessor accessor;
	mResourceById.find(accessor, id);
	ASSERT(!accessor.empty());
	mResourceById.erase(accessor);
	accessor.release();
}

void ResourceManager::EraseUploadBuffer(const std::size_t id) noexcept {
	UploadBufferById::accessor accessor;
	mUploadBufferById.find(accessor, id);
	ASSERT(!accessor.empty());
	mUploadBufferById.erase(accessor);
	accessor.release();
}

void ResourceManager::EraseFence(const std::size_t id) noexcept {
	FenceById::accessor accessor;
	mFenceById.find(accessor, id);
	ASSERT(!accessor.empty());
	mFenceById.erase(accessor);
	accessor.release();
}

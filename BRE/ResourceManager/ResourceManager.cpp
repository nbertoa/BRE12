#include "ResourceManager.h"

#include <memory>

#include <DirectXManager/DirectXManager.h>
#include <DXUtils/d3dx12.h>
#include <ResourceManager\DDSTextureLoader.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <SettingsManager\SettingsManager.h>
#include <Utils/DebugUtils.h>
#include <Utils/NumberGeneration.h>
#include <Utils\StringUtils.h>

ResourceManager::ResourceById ResourceManager::mResourceById;
std::mutex ResourceManager::mMutex;

std::size_t ResourceManager::LoadTextureFromFile(
	const char* textureFilename, 
	ID3D12GraphicsCommandList& commandList,
	ID3D12Resource* &resource, 
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept 
{
	ASSERT(textureFilename != nullptr);
	std::string filePath(SettingsManager::sResourcesPath);
	filePath += textureFilename;

	const std::wstring filePathW(StringUtils::ToWideString(filePath));

	Microsoft::WRL::ComPtr<ID3D12Resource> resourcePtr;

	mMutex.lock();
	CHECK_HR(DirectX::CreateDDSTextureFromFile12(
		&DirectXManager::GetDevice(), 
		&commandList, 
		filePathW.c_str(), 
		resourcePtr,
		uploadBuffer));
	mMutex.unlock();

	const std::size_t id{ NumberGeneration::GetIncrementalSizeT() };
	ResourceById::accessor accessor;
#ifdef _DEBUG
	mResourceById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mResourceById.insert(accessor, id);
	accessor->second = resourcePtr;
	accessor.release();

	resource = resourcePtr.Get();

	return id;
}

std::size_t ResourceManager::CreateDefaultBuffer(
	ID3D12GraphicsCommandList& commandList,
	const void* sourceData,
	const std::size_t sourceDataSize,
	ID3D12Resource* &buffer,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept
{
	ASSERT(sourceData != nullptr);
	ASSERT(sourceDataSize > 0);
	
	// Create the actual default buffer resource.
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1U;
	heapProps.VisibleNodeMask = 1U;

	CD3DX12_RESOURCE_DESC resDesc{ CD3DX12_RESOURCE_DESC::Buffer(sourceDataSize) };
	mMutex.lock();
	CHECK_HR(DirectXManager::GetDevice().CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&buffer)));

	// In order to copy CPU memory data into our default buffer, we need to create
	// an intermediate upload heap. 
	heapProps = D3D12_HEAP_PROPERTIES{};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1U;
	heapProps.VisibleNodeMask = 1U;
	resDesc = CD3DX12_RESOURCE_DESC::Buffer(sourceDataSize);

	CHECK_HR(DirectXManager::GetDevice().CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf())));
	mMutex.unlock();

	// Describe the data we want to copy into the default buffer.
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = sourceData;
	subResourceData.RowPitch = sourceDataSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	// Schedule to copy the data to the default buffer resource. At a high level, the helper function UpdateSubresources
	// will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
	// the intermediate upload heap data will be copied to mBuffer.
	CD3DX12_RESOURCE_BARRIER resBarrier{ CD3DX12_RESOURCE_BARRIER::Transition(buffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST) };
	commandList.ResourceBarrier(1, &resBarrier);
	UpdateSubresources<1>(&commandList, buffer, uploadBuffer.Get(), 0, 0, 1, &subResourceData);
	resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(buffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	commandList.ResourceBarrier(1, &resBarrier);

	const std::size_t id{ NumberGeneration::GetIncrementalSizeT() };
	ResourceById::accessor accessor;
#ifdef _DEBUG
	mResourceById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mResourceById.insert(accessor, id);
	accessor->second = Microsoft::WRL::ComPtr<ID3D12Resource>(buffer);
	accessor.release();

	return id;
}

std::size_t ResourceManager::CreateCommittedResource(
	const D3D12_HEAP_PROPERTIES& heapProperties,
	const D3D12_HEAP_FLAGS& heapFlags,
	const D3D12_RESOURCE_DESC& resourceDescriptor,
	const D3D12_RESOURCE_STATES& resourceStates,
	const D3D12_CLEAR_VALUE* clearValue,
	ID3D12Resource* &resource) noexcept
{
	mMutex.lock();
	CHECK_HR(DirectXManager::GetDevice().CreateCommittedResource(&heapProperties, heapFlags, &resourceDescriptor, resourceStates, clearValue, IID_PPV_ARGS(&resource)));
	mMutex.unlock();

	const std::size_t id{ NumberGeneration::GetIncrementalSizeT() };
	ResourceById::accessor accessor;
#ifdef _DEBUG
	mResourceById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mResourceById.insert(accessor, id);
	accessor->second = Microsoft::WRL::ComPtr<ID3D12Resource>(resource);
	accessor.release();

	ResourceStateManager::AddResource(*resource, resourceStates);

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

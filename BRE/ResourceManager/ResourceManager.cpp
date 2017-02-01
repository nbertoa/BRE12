#include "ResourceManager.h"

#include <DirectXManager/DirectXManager.h>
#include <DXUtils/d3dx12.h>
#include <ResourceManager\DDSTextureLoader.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <SettingsManager\SettingsManager.h>
#include <Utils/DebugUtils.h>
#include <Utils\StringUtils.h>

ResourceManager::Resources ResourceManager::mResources;
std::mutex ResourceManager::mMutex;

ResourceManager::~ResourceManager() {
	for (ID3D12Resource* resource : mResources) {
		ASSERT(resource != nullptr);
		resource->Release();
		delete resource;
	}
}

ID3D12Resource& ResourceManager::LoadTextureFromFile(
	const char* textureFilename, 
	ID3D12GraphicsCommandList& commandList,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer,
	const wchar_t* resourceName) noexcept
{
	ID3D12Resource* resource{ nullptr };

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

	resource = resourcePtr.Detach();

	ASSERT(resource != nullptr);
	mResources.insert(resource);

	if (resourceName != nullptr) {
		resource->SetName(resourceName);
	}
	
	return *resource;
}

ID3D12Resource& ResourceManager::CreateDefaultBuffer(
	ID3D12GraphicsCommandList& commandList,
	const void* sourceData,
	const std::size_t sourceDataSize,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer,
	const wchar_t* resourceName) noexcept
{
	ASSERT(sourceData != nullptr);
	ASSERT(sourceDataSize > 0);

	ID3D12Resource* resource{ nullptr };
	
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
		IID_PPV_ARGS(&resource)));

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
	CD3DX12_RESOURCE_BARRIER resBarrier{ CD3DX12_RESOURCE_BARRIER::Transition(
		resource, 
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST) };
	commandList.ResourceBarrier(1, &resBarrier);
	UpdateSubresources<1>(&commandList, resource, uploadBuffer.Get(), 0, 0, 1, &subResourceData);
	resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	commandList.ResourceBarrier(1, &resBarrier);

	ASSERT(resource != nullptr);
	mResources.insert(resource);

	if (resourceName != nullptr) {
		resource->SetName(resourceName);
	}

	return *resource;
}

ID3D12Resource& ResourceManager::CreateCommittedResource(
	const D3D12_HEAP_PROPERTIES& heapProperties,
	const D3D12_HEAP_FLAGS& heapFlags,
	const D3D12_RESOURCE_DESC& resourceDescriptor,
	const D3D12_RESOURCE_STATES& resourceStates,
	const D3D12_CLEAR_VALUE* clearValue,
	const wchar_t* resourceName) noexcept
{
	ID3D12Resource* resource{ nullptr };

	mMutex.lock();
	CHECK_HR(DirectXManager::GetDevice().CreateCommittedResource(
		&heapProperties, 
		heapFlags, 
		&resourceDescriptor, 
		resourceStates, 
		clearValue, 
		IID_PPV_ARGS(&resource)));
	mMutex.unlock();

	ResourceStateManager::AddResource(*resource, resourceStates);

	ASSERT(resource != nullptr);
	mResources.insert(resource);

	if (resourceName != nullptr) {
		resource->SetName(resourceName);
	}

	return *resource;
}

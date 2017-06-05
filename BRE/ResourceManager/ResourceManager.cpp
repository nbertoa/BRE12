#include "ResourceManager.h"

#include <DirectXManager/DirectXManager.h>
#include <DXUtils\D3DFactory.h>
#include <ResourceManager\DDSTextureLoader.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <ApplicationSettings\ApplicationSettings.h>
#include <Utils/DebugUtils.h>
#include <Utils\StringUtils.h>

namespace BRE {
tbb::concurrent_unordered_set<ID3D12Resource*> ResourceManager::mResources;
std::mutex ResourceManager::mMutex;

void
ResourceManager::Clear() noexcept
{
    for (ID3D12Resource* resource : mResources) {
        BRE_ASSERT(resource != nullptr);
        resource->Release();
    }

    mResources.clear();
}

ID3D12Resource&
ResourceManager::LoadTextureFromFile(const char* textureFilename,
                                     ID3D12GraphicsCommandList& commandList,
                                     ID3D12Resource* &uploadBuffer,
                                     const wchar_t* resourceName) noexcept
{
    ID3D12Resource* resource{ nullptr };

    BRE_ASSERT(textureFilename != nullptr);
    const std::string filePath(textureFilename);
    const std::wstring filePathW(StringUtils::AnsiToWideString(filePath));

    Microsoft::WRL::ComPtr<ID3D12Resource> resourcePtr;
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBufferPtr;
    mMutex.lock();
    BRE_CHECK_HR(DirectX::CreateDDSTextureFromFile12(&DirectXManager::GetDevice(),
                                                     &commandList,
                                                     filePathW.c_str(),
                                                     resourcePtr,
                                                     uploadBufferPtr));
    mMutex.unlock();

    uploadBuffer = uploadBufferPtr.Detach();

    resource = resourcePtr.Detach();

    BRE_ASSERT(resource != nullptr);
    mResources.insert(resource);

    if (resourceName != nullptr) {
        resource->SetName(resourceName);
    }

    return *resource;
}

ID3D12Resource&
ResourceManager::CreateDefaultBuffer(const void* sourceData,
                                     const std::size_t sourceDataSize,
                                     ID3D12GraphicsCommandList& commandList,
                                     ID3D12Resource* &uploadBuffer,
                                     const wchar_t* resourceName) noexcept
{
    BRE_ASSERT(sourceData != nullptr);
    BRE_ASSERT(sourceDataSize > 0);

    ID3D12Resource* resource{ nullptr };

    // Create the actual default buffer resource.
    D3D12_HEAP_PROPERTIES heapProperties = D3DFactory::GetHeapProperties(D3D12_HEAP_TYPE_DEFAULT,
                                                                         D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                                                                         D3D12_MEMORY_POOL_UNKNOWN,
                                                                         1U,
                                                                         1U);

    const D3D12_RESOURCE_DESC resourceDescriptor = D3DFactory::GetResourceDescriptor(sourceDataSize,
                                                                                     1,
                                                                                     DXGI_FORMAT_UNKNOWN,
                                                                                     D3D12_RESOURCE_FLAG_NONE,
                                                                                     D3D12_RESOURCE_DIMENSION_BUFFER,
                                                                                     D3D12_TEXTURE_LAYOUT_ROW_MAJOR);

    mMutex.lock();
    BRE_CHECK_HR(DirectXManager::GetDevice().CreateCommittedResource(&heapProperties,
                                                                     D3D12_HEAP_FLAG_NONE,
                                                                     &resourceDescriptor,
                                                                     D3D12_RESOURCE_STATE_COMMON,
                                                                     nullptr,
                                                                     IID_PPV_ARGS(&resource)));

    // In order to copy CPU memory data into our default buffer, we need to create
    // an intermediate upload heap. 
    heapProperties = D3DFactory::GetHeapProperties(D3D12_HEAP_TYPE_UPLOAD,
                                                   D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                                                   D3D12_MEMORY_POOL_UNKNOWN,
                                                   1U,
                                                   1U);

    BRE_CHECK_HR(DirectXManager::GetDevice().CreateCommittedResource(&heapProperties,
                                                                     D3D12_HEAP_FLAG_NONE,
                                                                     &resourceDescriptor,
                                                                     D3D12_RESOURCE_STATE_GENERIC_READ,
                                                                     nullptr,
                                                                     IID_PPV_ARGS(&uploadBuffer)));
    mMutex.unlock();

    // Describe the data we want to copy into the default buffer.
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = sourceData;
    subResourceData.RowPitch = sourceDataSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // Schedule to copy the data to the default buffer resource. At a high level, the helper function UpdateSubresources
    // will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
    // the intermediate upload heap data will be copied to mBuffer.
    D3D12_RESOURCE_BARRIER resourceBarrier = D3DFactory::GetTransitionResourceBarrier(*resource,
                                                                                      D3D12_RESOURCE_STATE_COMMON,
                                                                                      D3D12_RESOURCE_STATE_COPY_DEST); 
    commandList.ResourceBarrier(1, &resourceBarrier);

    UpdateSubresources<1>(&commandList, 
                          resource, 
                          uploadBuffer, 
                          0, 
                          0, 
                          1,
                          &subResourceData);

    resourceBarrier = D3DFactory::GetTransitionResourceBarrier(*resource,
                                                               D3D12_RESOURCE_STATE_COPY_DEST,
                                                               D3D12_RESOURCE_STATE_GENERIC_READ);
    commandList.ResourceBarrier(1, &resourceBarrier);

    BRE_ASSERT(resource != nullptr);
    mResources.insert(resource);

    if (resourceName != nullptr) {
        resource->SetName(resourceName);
    }

    return *resource;
}

ID3D12Resource&
ResourceManager::CreateCommittedResource(const D3D12_HEAP_PROPERTIES& heapProperties,
                                         const D3D12_HEAP_FLAGS& heapFlags,
                                         const D3D12_RESOURCE_DESC& resourceDescriptor,
                                         const D3D12_RESOURCE_STATES& resourceStates,
                                         const D3D12_CLEAR_VALUE* clearValue,
                                         const wchar_t* resourceName,
                                         const ResourceStateTrackingType resourceStateTrackingType) noexcept
{
    ID3D12Resource* resource{ nullptr };

    mMutex.lock();
    BRE_CHECK_HR(DirectXManager::GetDevice().CreateCommittedResource(&heapProperties,
                                                                     heapFlags,
                                                                     &resourceDescriptor,
                                                                     resourceStates,
                                                                     clearValue,
                                                                     IID_PPV_ARGS(&resource)));
    mMutex.unlock();

    switch (resourceStateTrackingType) {
    case ResourceStateTrackingType::FULL_TRACKING:
        ResourceStateManager::AddFullResourceTracking(*resource,
                                                      resourceStates);
        break;
    case ResourceStateTrackingType::NO_TRACKING:
        break;
    case ResourceStateTrackingType::SUBRESOURCE_TRACKING:
        ResourceStateManager::AddSubresourceTracking(*resource,
                                                     resourceStates);
        break;
    default:
        BRE_ASSERT(false && "Unknown ResourceStateTrackingType");
        break;
    };

    BRE_ASSERT(resource != nullptr);
    mResources.insert(resource);

    if (resourceName != nullptr) {
        resource->SetName(resourceName);
    }

    return *resource;
}
}
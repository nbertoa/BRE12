#include "CbvSrvUavDescriptorManager.h"

#include <memory>

#include <DirectXManager\DirectXManager.h>
#include <DXUtils/d3dx12.h>

namespace BRE {
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CbvSrvUavDescriptorManager::mCbvSrvUavDescriptorHeap;
D3D12_GPU_DESCRIPTOR_HANDLE CbvSrvUavDescriptorManager::mCurrentCbvSrvUavGpuDescriptorHandle{ 0UL };
D3D12_CPU_DESCRIPTOR_HANDLE CbvSrvUavDescriptorManager::mCurrentCbvSrvUavCpuDescriptorHandle{ 0UL };
std::mutex CbvSrvUavDescriptorManager::mMutex;

void
CbvSrvUavDescriptorManager::Init(const std::uint32_t numDescriptorsInCbvSrvUavDescriptorHeap) noexcept
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavDescriptorHeapDescriptor{};
    cbvSrvUavDescriptorHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvSrvUavDescriptorHeapDescriptor.NodeMask = 0U;
    cbvSrvUavDescriptorHeapDescriptor.NumDescriptors = numDescriptorsInCbvSrvUavDescriptorHeap;
    cbvSrvUavDescriptorHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    mMutex.lock();
    BRE_CHECK_HR(DirectXManager::GetDevice().CreateDescriptorHeap(&cbvSrvUavDescriptorHeapDescriptor,
                                                                  IID_PPV_ARGS(mCbvSrvUavDescriptorHeap.GetAddressOf())));
    mMutex.unlock();

    mCurrentCbvSrvUavGpuDescriptorHandle = mCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    mCurrentCbvSrvUavCpuDescriptorHandle = mCbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE
CbvSrvUavDescriptorManager::CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& descriptor) noexcept
{
    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

    mMutex.lock();
    gpuDescriptorHandle = mCurrentCbvSrvUavGpuDescriptorHandle;

    DirectXManager::GetDevice().CreateConstantBufferView(&descriptor, mCurrentCbvSrvUavCpuDescriptorHandle);

    mCurrentCbvSrvUavGpuDescriptorHandle.ptr +=
        DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    mCurrentCbvSrvUavCpuDescriptorHandle.ptr +=
        DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    mMutex.unlock();

    return gpuDescriptorHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE
CbvSrvUavDescriptorManager::CreateConstantBufferViews(const D3D12_CONSTANT_BUFFER_VIEW_DESC* descriptors,
                                                      const std::uint32_t descriptorCount) noexcept
{
    BRE_ASSERT(descriptors != nullptr);
    BRE_ASSERT(descriptorCount > 0U);

    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

    mMutex.lock();
    gpuDescriptorHandle = mCurrentCbvSrvUavGpuDescriptorHandle;

    for (std::uint32_t i = 0U; i < descriptorCount; ++i) {
        DirectXManager::GetDevice().CreateConstantBufferView(&descriptors[i], mCurrentCbvSrvUavCpuDescriptorHandle);
        mCurrentCbvSrvUavCpuDescriptorHandle.ptr +=
            DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    mCurrentCbvSrvUavGpuDescriptorHandle.ptr +=
        descriptorCount * DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    mMutex.unlock();

    return gpuDescriptorHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE
CbvSrvUavDescriptorManager::CreateShaderResourceView(ID3D12Resource& resource,
                                                     const D3D12_SHADER_RESOURCE_VIEW_DESC& descriptor) noexcept
{
    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

    mMutex.lock();
    gpuDescriptorHandle = mCurrentCbvSrvUavGpuDescriptorHandle;

    DirectXManager::GetDevice().CreateShaderResourceView(&resource,
                                                         &descriptor,
                                                         mCurrentCbvSrvUavCpuDescriptorHandle);

    mCurrentCbvSrvUavGpuDescriptorHandle.ptr +=
        DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    mCurrentCbvSrvUavCpuDescriptorHandle.ptr +=
        DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    mMutex.unlock();

    return gpuDescriptorHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE
CbvSrvUavDescriptorManager::CreateShaderResourceViews(ID3D12Resource* *resources,
                                                      const D3D12_SHADER_RESOURCE_VIEW_DESC* descriptors,
                                                      const std::uint32_t descriptorCount) noexcept
{
    BRE_ASSERT(resources != nullptr);
    BRE_ASSERT(descriptors != nullptr);
    BRE_ASSERT(descriptorCount > 0U);

    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

    mMutex.lock();
    gpuDescriptorHandle = mCurrentCbvSrvUavGpuDescriptorHandle;

    for (std::uint32_t i = 0U; i < descriptorCount; ++i) {
        BRE_ASSERT(resources[i] != nullptr);
        DirectXManager::GetDevice().CreateShaderResourceView(resources[i],
                                                             &descriptors[i],
                                                             mCurrentCbvSrvUavCpuDescriptorHandle);
        mCurrentCbvSrvUavCpuDescriptorHandle.ptr +=
            DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    mCurrentCbvSrvUavGpuDescriptorHandle.ptr +=
        descriptorCount * DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    mMutex.unlock();

    return gpuDescriptorHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE
CbvSrvUavDescriptorManager::CreateUnorderedAccessView(ID3D12Resource& resource,
                                                      const D3D12_UNORDERED_ACCESS_VIEW_DESC& descriptor) noexcept
{
    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

    mMutex.lock();
    gpuDescriptorHandle = mCurrentCbvSrvUavGpuDescriptorHandle;

    DirectXManager::GetDevice().CreateUnorderedAccessView(&resource,
                                                          nullptr,
                                                          &descriptor,
                                                          mCurrentCbvSrvUavCpuDescriptorHandle);

    mCurrentCbvSrvUavGpuDescriptorHandle.ptr +=
        DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    mCurrentCbvSrvUavCpuDescriptorHandle.ptr +=
        DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    mMutex.unlock();

    return gpuDescriptorHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE
CbvSrvUavDescriptorManager::CreateUnorderedAccessViews(ID3D12Resource* *resources,
                                                       const D3D12_UNORDERED_ACCESS_VIEW_DESC* descriptors,
                                                       const std::uint32_t descriptorCount) noexcept
{
    BRE_ASSERT(resources != nullptr);
    BRE_ASSERT(descriptors != nullptr);
    BRE_ASSERT(descriptorCount > 0U);

    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

    mMutex.lock();
    gpuDescriptorHandle = mCurrentCbvSrvUavGpuDescriptorHandle;

    for (std::uint32_t i = 0U; i < descriptorCount; ++i) {
        BRE_ASSERT(resources[i] != nullptr);
        DirectXManager::GetDevice().CreateUnorderedAccessView(resources[i],
                                                              nullptr,
                                                              &descriptors[i],
                                                              mCurrentCbvSrvUavCpuDescriptorHandle);
        mCurrentCbvSrvUavCpuDescriptorHandle.ptr +=
            DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    mCurrentCbvSrvUavGpuDescriptorHandle.ptr +=
        descriptorCount * DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    mMutex.unlock();

    return gpuDescriptorHandle;
}
}
#include "ResourceManager.h"

#include <DXUtils/d3dx12.h>
#include <Utils/DebugUtils.h>

std::unique_ptr<ResourceManager> ResourceManager::gResourceMgr = nullptr;

size_t ResourceManager::CreateDefaultBuffer(
	ID3D12Device& device,
	ID3D12GraphicsCommandList& cmdList,
	const void* initData,
	const uint64_t byteSize,
	Microsoft::WRL::ComPtr<ID3D12Resource>& defaultBuffer,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
	ASSERT(initData);
	ASSERT(byteSize > 0);

	const size_t id{ mResources.size() };

	// Create the actual default buffer resource.
	CD3DX12_HEAP_PROPERTIES heapProps{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };
	CD3DX12_RESOURCE_DESC resDesc{ CD3DX12_RESOURCE_DESC::Buffer(byteSize) };
	CHECK_HR(device.CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

	// In order to copy CPU memory data into our default buffer, we need to create
	// an intermediate upload heap. 
	heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	resDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	CHECK_HR(device.CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

	// Describe the data we want to copy into the default buffer.
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	// Schedule to copy the data to the default buffer resource. At a high level, the helper function UpdateSubresources
	// will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
	// the intermediate upload heap data will be copied to mBuffer.
	CD3DX12_RESOURCE_BARRIER resBarrier{ CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST) };
	cmdList.ResourceBarrier(1, &resBarrier);
	UpdateSubresources<1>(&cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
	resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	cmdList.ResourceBarrier(1, &resBarrier);

	mResources.push_back(defaultBuffer);

	return id;
}

Microsoft::WRL::ComPtr<ID3D12Resource> ResourceManager::GetResource(const size_t id) {
	ASSERT(id < mResources.size());
	return mResources[id];
}

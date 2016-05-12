#include "ResourceManager.h"

#include <DXUtils/d3dx12.h>
#include <Utils/DebugUtils.h>

std::unique_ptr<ResourceManager> ResourceManager::gManager = nullptr;

std::size_t ResourceManager::CreateDefaultBuffer(
	const std::string& name,
	ID3D12GraphicsCommandList& cmdList,
	const void* initData,
	const std::uint64_t byteSize,
	Microsoft::WRL::ComPtr<ID3D12Resource>& defaultBuffer,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept
{
	ASSERT(initData);
	ASSERT(byteSize > 0);
	ASSERT(!name.empty());

	const std::size_t id{ mHash(name) };

	ASSERT(mResourceById.find(id) == mResourceById.end());

	// Create the actual default buffer resource.
	CD3DX12_HEAP_PROPERTIES heapProps{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };
	CD3DX12_RESOURCE_DESC resDesc{ CD3DX12_RESOURCE_DESC::Buffer(byteSize) };
	CHECK_HR(mDevice.CreateCommittedResource(
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
	CHECK_HR(mDevice.CreateCommittedResource(
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

	mResourceById.insert(IdAndResource{ id, defaultBuffer });

	return id;
}

std::size_t ResourceManager::CreateUploadBuffer(const std::string& name, const std::size_t elemSize, const std::uint32_t elemCount, UploadBuffer*& buffer) noexcept {
	ASSERT(!name.empty());
	const std::size_t id{ mHash(name) };
	ASSERT(mUploadBufferById.find(id) == mUploadBufferById.end());

	std::unique_ptr<UploadBuffer>& uploadBuffer = mUploadBufferById[id];
	uploadBuffer = std::make_unique<UploadBuffer>(mDevice, elemSize, elemCount);
	buffer = uploadBuffer.get();

	return id;
}

Microsoft::WRL::ComPtr<ID3D12Resource> ResourceManager::GetResource(const std::size_t id) noexcept {
	ResourceById::iterator it{ mResourceById.find(id) };
	ASSERT(it != mResourceById.end());

	return it->second;
}

UploadBuffer* ResourceManager::GetUploadBuffer(const size_t id) noexcept {
	UploadBufferById::iterator it{ mUploadBufferById.find(id) };
	ASSERT(it != mUploadBufferById.end());

	return it->second.get();
}

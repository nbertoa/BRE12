#include "ResourceManager.h"

#include <DXUtils/d3dx12.h>
#include <Utils/DebugUtils.h>

std::unique_ptr<ResourceManager> ResourceManager::gManager = nullptr;

std::size_t ResourceManager::CreateDefaultBuffer(
	ID3D12GraphicsCommandList& cmdList,
	const void* initData,
	const std::size_t byteSize,
	ID3D12Resource* &defaultBuffer,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept
{
	ASSERT(initData != nullptr);
	ASSERT(byteSize > 0);
	
	const std::size_t id{ mRandGen.RandomNumber() };

	ResourceById::accessor accessor;

#ifdef _DEBUG
	mResourceById.find(accessor, id);
	ASSERT(accessor.empty());
#endif

	// Create the actual default buffer resource.
	CD3DX12_HEAP_PROPERTIES heapProps{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };
	CD3DX12_RESOURCE_DESC resDesc{ CD3DX12_RESOURCE_DESC::Buffer(byteSize) };

	mMutex.lock();
	CHECK_HR(mDevice.CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&defaultBuffer)));
	mMutex.unlock();

	// In order to copy CPU memory data into our default buffer, we need to create
	// an intermediate upload heap. 
	heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	resDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

	mMutex.lock();
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

	mResourceById.insert(accessor, id);
	accessor->second = Microsoft::WRL::ComPtr<ID3D12Resource>(defaultBuffer);
	accessor.release();

	return id;
}

std::size_t ResourceManager::CreateUploadBuffer(const std::size_t elemSize, const std::uint32_t elemCount, UploadBuffer*& buffer) noexcept {
	const std::size_t id{ mRandGen.RandomNumber() };

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
	ID3D12Resource* res{ accessor->second.Get() };
	accessor.release();

	return *res;
}

UploadBuffer& ResourceManager::GetUploadBuffer(const size_t id) noexcept {
	UploadBufferById::accessor accessor;
	mUploadBufferById.find(accessor, id);
	ASSERT(!accessor.empty());
	UploadBuffer* buf{ accessor->second.get() };
	accessor.release();

	return *buf;
}

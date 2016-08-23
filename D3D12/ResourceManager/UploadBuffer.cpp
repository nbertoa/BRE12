#include "UploadBuffer.h"

#include <DxUtils/d3dx12.h>
#include <Utils/DebugUtils.h>

UploadBuffer::UploadBuffer(ID3D12Device& device, const std::size_t elemSize, const std::uint32_t elemCount)
	: mElemSize(elemSize)
{
	ASSERT(elemSize > 0);
	ASSERT(elemCount > 0);

	CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
	CD3DX12_RESOURCE_DESC resDesc{ CD3DX12_RESOURCE_DESC::Buffer(mElemSize * elemCount) };
	CHECK_HR(device.CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mBuffer)));
	CHECK_HR(mBuffer->Map(0, nullptr, (void**)&mMappedData));
}

UploadBuffer::~UploadBuffer() {
	ASSERT(mBuffer);
	ASSERT(mElemSize > 0);
	mBuffer->Unmap(0, nullptr);
	mMappedData = nullptr;
}

void UploadBuffer::CopyData(const std::uint32_t elemIndex, const void* srcData, const std::size_t srcDataSize) noexcept {
	ASSERT(srcData);
	memcpy(mMappedData + elemIndex * mElemSize, srcData, srcDataSize);
}

std::size_t UploadBuffer::CalcConstantBufferByteSize(const std::size_t byteSize) {
	// Constant buffers must be a multiple of the minimum hardware
	// allocation size (usually 256 bytes).  So round up to nearest
	// multiple of 256.  We do this by adding 255 and then masking off
	// the lower 2 bytes which store all bits < 256.
	// Example: Suppose byteSize = 300.
	// (300 + 255) & ~255
	// 555 & ~255
	// 0x022B & ~0x00ff
	// 0x022B & 0xff00
	// 0x0200
	// 512
	return (byteSize + 255U) & ~255U;
}
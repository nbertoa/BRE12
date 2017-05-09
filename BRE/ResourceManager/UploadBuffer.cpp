#include "UploadBuffer.h"

#include <DxUtils/d3dx12.h>
#include <Utils/DebugUtils.h>

namespace BRE {
UploadBuffer::UploadBuffer(ID3D12Device& device,
                           const std::size_t elementSize,
                           const std::uint32_t elementCount)
    : mElementSize(elementSize)
{
    BRE_ASSERT(elementSize > 0);
    BRE_ASSERT(elementCount > 0);

    CD3DX12_HEAP_PROPERTIES heapProperties{ D3D12_HEAP_TYPE_UPLOAD };
    CD3DX12_RESOURCE_DESC resourceDescriptor{ CD3DX12_RESOURCE_DESC::Buffer(mElementSize * elementCount) };
    BRE_CHECK_HR(device.CreateCommittedResource(&heapProperties,
                                                D3D12_HEAP_FLAG_NONE,
                                                &resourceDescriptor,
                                                D3D12_RESOURCE_STATE_GENERIC_READ,
                                                nullptr,
                                                IID_PPV_ARGS(&mBuffer)));
    BRE_CHECK_HR(mBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));
}

UploadBuffer::~UploadBuffer()
{
    BRE_ASSERT(mBuffer);
    BRE_ASSERT(mElementSize > 0);
    mBuffer->Unmap(0, nullptr);
    mMappedData = nullptr;
}

void
UploadBuffer::CopyData(const std::uint32_t elementIndex,
                       const void* sourceData,
                       const std::size_t sourceDataSize) const noexcept
{
    BRE_ASSERT(sourceData);
    memcpy(mMappedData + elementIndex * mElementSize, sourceData, sourceDataSize);
}

std::size_t
UploadBuffer::GetRoundedConstantBufferSizeInBytes(const std::size_t sizeInBytes)
{
    // Constant buffers must be a multiple of the minimum hardware
    // allocation size (usually 256 bytes).  So round up to nearest
    // multiple of 256.  We do this by adding 255 and then masking off
    // the lower 2 bytes which store all bits < 256.
    // Example: Suppose sizeInBytes = 300.
    // (300 + 255) & ~255
    // 555 & ~255
    // 0x022B & ~0x00ff
    // 0x022B & 0xff00
    // 0x0200
    // 512
    return (sizeInBytes + 255U) & ~255U;
}
}


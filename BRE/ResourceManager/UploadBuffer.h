#pragma once

#include <cstddef>
#include <cstdint>
#include <d3d12.h>
#include <wrl.h>

class UploadBuffer {
public:
    explicit UploadBuffer(ID3D12Device& device,
                          const std::size_t elementSize,
                          const std::uint32_t elementCount);

    ~UploadBuffer();
    UploadBuffer(const UploadBuffer&) = delete;
    const UploadBuffer& operator=(const UploadBuffer&) = delete;
    UploadBuffer(UploadBuffer&&) = delete;
    UploadBuffer& operator=(UploadBuffer&&) = delete;

    __forceinline ID3D12Resource* GetResource() const noexcept
    {
        return mBuffer.Get();
    }

    void CopyData(const std::uint32_t elementIndex,
                  const void* sourceData,
                  const std::size_t sourceDataSize) const noexcept;

    // Constant buffers must be a multiple of the minimum hardware
    // allocation size (usually 256 bytes). So round up to nearest
    // multiple of 256.
    static std::size_t GetRoundedConstantBufferSizeInBytes(const std::size_t sizeInBytes);

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> mBuffer;
    std::uint8_t* mMappedData{ nullptr };
    std::size_t mElementSize{ 0U };
};
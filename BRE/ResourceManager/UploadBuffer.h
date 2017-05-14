#pragma once

#include <cstddef>
#include <cstdint>
#include <d3d12.h>
#include <wrl.h>

namespace BRE {
///
/// @brief Represents a buffer to upload data to GPU
///
class UploadBuffer {
public:
    ///
    /// @brief Upload buffer constructor
    /// @param device Device needed to create the buffer
    /// @param elementSize Size in bytes of the type of element in the buffer
    /// @param elementCount Number of elements in the buffer
    ///
    explicit UploadBuffer(ID3D12Device& device,
                          const std::size_t elementSize,
                          const std::uint32_t elementCount);

    ~UploadBuffer();
    UploadBuffer(const UploadBuffer&) = delete;
    const UploadBuffer& operator=(const UploadBuffer&) = delete;
    UploadBuffer(UploadBuffer&&) = delete;
    UploadBuffer& operator=(UploadBuffer&&) = delete;

    ///
    /// @brief Get resource
    /// @return Resource
    ///
    __forceinline ID3D12Resource* GetResource() const noexcept
    {
        return mBuffer.Get();
    }

    ///
    /// @brief Copy data
    /// @param elementIndex Element index to copy
    /// @param sourceData Source data to copy to the element in @p elementIndex. Must not be nullptr
    /// @param sourceDataSize Size of the source data. Must be greater than zero.
    ///
    void CopyData(const std::uint32_t elementIndex,
                  const void* sourceData,
                  const std::size_t sourceDataSize) const noexcept;

    ///
    /// @brief Get rounded constant buffer size in bytes
    ///
    /// Constant buffers must be a multiple of the minimum hardware
    /// allocation size (usually 256 bytes). So round up to nearest
    /// multiple of 256.
    ///
    /// @param sizeInBytes Size in bytes of the data. Must be greater than zero
    /// @return Rounded size
    ///
    static std::size_t GetRoundedConstantBufferSizeInBytes(const std::size_t sizeInBytes);

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> mBuffer;
    std::uint8_t* mMappedData{ nullptr };
    std::size_t mElementSize{ 0U };
};
}


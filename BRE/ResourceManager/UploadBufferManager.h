#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb/concurrent_unordered_set.h>

#include <ResourceManager/UploadBuffer.h>

namespace BRE {
///
/// @brief Responsible to create upload buffers
///
class UploadBufferManager {
public:
    UploadBufferManager() = delete;
    ~UploadBufferManager() = delete;
    UploadBufferManager(const UploadBufferManager&) = delete;
    const UploadBufferManager& operator=(const UploadBufferManager&) = delete;
    UploadBufferManager(UploadBufferManager&&) = delete;
    UploadBufferManager& operator=(UploadBufferManager&&) = delete;

    ///
    /// @brief Releases all upload buffers
    ///
    static void Clear() noexcept;

    ///
    /// @brief Creates upload buffer
    /// @param elementSize Size of the element in the upload buffer. Must be greater than zero
    /// @param elementCount Number of elements in the upload buffer. Must be greater than zero.
    /// @return Upload buffer
    ///
    static UploadBuffer& CreateUploadBuffer(const std::size_t elementSize,
                                            const std::uint32_t elementCount) noexcept;

private:
    static tbb::concurrent_unordered_set<UploadBuffer*> mUploadBuffers;

    static std::mutex mMutex;
};
}
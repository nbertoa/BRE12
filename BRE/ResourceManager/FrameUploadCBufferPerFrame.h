#pragma once

#include <cstdint>

#include <ResourceManager\UploadBuffer.h>
#include <ApplicationSettings\ApplicationSettings.h>

namespace BRE {
///
/// @brief Provides a frame constant buffer per frame
///
/// We support to have different number of queued frames.
///
class FrameUploadCBufferPerFrame {
public:
    FrameUploadCBufferPerFrame();
    ~FrameUploadCBufferPerFrame() = default;
    FrameUploadCBufferPerFrame(const FrameUploadCBufferPerFrame&) = delete;
    const FrameUploadCBufferPerFrame& operator=(const FrameUploadCBufferPerFrame&) = delete;
    FrameUploadCBufferPerFrame(FrameUploadCBufferPerFrame&&) = default;
    FrameUploadCBufferPerFrame& operator=(FrameUploadCBufferPerFrame&&) = default;

    ///
    /// @brief Get next constant buffer per frame
    /// @return Upload buffer
    ///
    UploadBuffer& GetNextFrameCBuffer() noexcept;

private:
    UploadBuffer* mFrameCBuffers[ApplicationSettings::sQueuedFrameCount];
    std::uint32_t mCurrentFrameIndex{ 0U };
};

}


#pragma once

#include <cstdint>

#include <ResourceManager\UploadBuffer.h>
#include <SettingsManager\SettingsManager.h>

namespace BRE {
// We support to have different number of queued frames.
// This class provides a frame constant buffer per frame.
class FrameUploadCBufferPerFrame {
public:
    FrameUploadCBufferPerFrame();
    ~FrameUploadCBufferPerFrame() = default;
    FrameUploadCBufferPerFrame(const FrameUploadCBufferPerFrame&) = delete;
    const FrameUploadCBufferPerFrame& operator=(const FrameUploadCBufferPerFrame&) = delete;
    FrameUploadCBufferPerFrame(FrameUploadCBufferPerFrame&&) = default;
    FrameUploadCBufferPerFrame& operator=(FrameUploadCBufferPerFrame&&) = default;

    UploadBuffer& GetNextFrameCBuffer() noexcept;

private:
    UploadBuffer* mFrameCBuffers[SettingsManager::sQueuedFrameCount];
    std::uint32_t mCurrentFrameIndex{ 0U };
};

}


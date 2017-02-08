#pragma once

#include <cstdint>

#include <ResourceManager\UploadBuffer.h>
#include <SettingsManager\SettingsManager.h>

// We support to have different number of queued frames.
// This class provides a frame constant buffer per frame.
class FrameCBufferPerFrame {
public:
	FrameCBufferPerFrame();
	~FrameCBufferPerFrame() = default;
	FrameCBufferPerFrame(const FrameCBufferPerFrame&) = delete;
	const FrameCBufferPerFrame& operator=(const FrameCBufferPerFrame&) = delete;
	FrameCBufferPerFrame(FrameCBufferPerFrame&&) = default;
	FrameCBufferPerFrame& operator=(FrameCBufferPerFrame&&) = default;

	UploadBuffer& GetNextFrameCBuffer() noexcept;

private:
	UploadBuffer* mFrameCBuffers[SettingsManager::sQueuedFrameCount];
	std::uint32_t mCurrentFrameIndex{ 0U };
};

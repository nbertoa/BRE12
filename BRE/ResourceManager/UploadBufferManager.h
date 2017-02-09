#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb/concurrent_unordered_set.h>

#include <ResourceManager/UploadBuffer.h>

// This class is responsible to create/get upload buffers
class UploadBufferManager {
public:
	UploadBufferManager() = delete;
	~UploadBufferManager() = delete;
	UploadBufferManager(const UploadBufferManager&) = delete;
	const UploadBufferManager& operator=(const UploadBufferManager&) = delete;
	UploadBufferManager(UploadBufferManager&&) = delete;
	UploadBufferManager& operator=(UploadBufferManager&&) = delete;

	static void EraseAll() noexcept;
	
	// Preconditions:
	// - "elementSize" must be greater than zero.
	// - "elementCount" must be greater than zero.
	static UploadBuffer& CreateUploadBuffer(
		const std::size_t elementSize,
		const std::uint32_t elementCount) noexcept;

private:
	using UploadBuffers = tbb::concurrent_unordered_set<UploadBuffer*>;
	static UploadBuffers mUploadBuffers;

	static std::mutex mMutex;
};

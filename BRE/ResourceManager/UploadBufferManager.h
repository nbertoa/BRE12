#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb/concurrent_hash_map.h>
#include <wrl.h>

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
	
	// Preconditions:
	// - "elementSize" must be greater than zero.
	// - "elementCount" must be greater than zero.
	static std::size_t CreateUploadBuffer(
		const std::size_t elementSize,
		const std::uint32_t elementCount,
		UploadBuffer*& uploadBuffer) noexcept;

	// Preconditions:
	// - "id" must be valid.
	static UploadBuffer& GetUploadBuffer(const std::size_t id) noexcept;

private:
	using UploadBufferById = tbb::concurrent_hash_map<std::size_t, std::unique_ptr<UploadBuffer>>;
	static UploadBufferById mUploadBufferById;

	static std::mutex mMutex;
};

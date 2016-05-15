#pragma once

#include <cstdint>
#include <functional>
#include <d3d12.h>
#include <memory>
#include <unordered_map>
#include <wrl.h>

#include "UploadBuffer.h"

class ResourceManager {
public:
	static std::unique_ptr<ResourceManager> gManager;

	explicit ResourceManager(ID3D12Device& device) : mDevice(device) {}
	ResourceManager(const ResourceManager&) = delete;
	const ResourceManager& operator=(const ResourceManager&) = delete;

	// Returns resource id.
	// Note: uploadBuffer has to be kept alive after the above function calls because
	// the command list has not been executed yet that performs the actual copy.
	// The caller can Release the uploadBuffer after it knows the copy has been executed.
	//
	// Asserts if resource with the same name was already registered
	std::size_t CreateDefaultBuffer(
		const std::string& name,		
		ID3D12GraphicsCommandList& cmdList,
		const void* initData,
		const std::size_t byteSize,
		ID3D12Resource* &defaultBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept;

	// Asserts if resource with the same name was already registered
	std::size_t CreateUploadBuffer(const std::string& name, const std::size_t elemSize, const std::uint32_t elemCount, UploadBuffer*& buffer) noexcept;

	// Asserts if resource id is not present
	ID3D12Resource& GetResource(const std::size_t id) noexcept;
	UploadBuffer& GetUploadBuffer(const size_t id) noexcept;

	// This will invalidate all ids.
	void ClearDefaultBuffers() noexcept { mResourceById.clear(); }
	void ClearUploadBuffers() noexcept { mUploadBufferById.clear(); }
	void Clear() noexcept { ClearDefaultBuffers(); ClearUploadBuffers(); }

private:
	ID3D12Device& mDevice;

	using IdAndResource = std::pair<std::size_t, Microsoft::WRL::ComPtr<ID3D12Resource>>;
	using ResourceById = std::unordered_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12Resource>>;
	ResourceById mResourceById;

	using IdAndUploadBuffer = std::pair<std::size_t, std::unique_ptr<UploadBuffer>>;
	using UploadBufferById = std::unordered_map<std::size_t, std::unique_ptr<UploadBuffer>>;
	UploadBufferById mUploadBufferById;

	std::hash<std::string> mHash;
};

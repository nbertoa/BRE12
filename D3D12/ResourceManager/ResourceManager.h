#pragma once

#include <cstdint>
#include <d3d12.h>
#include <vector>
#include <wrl.h>

class ResourceManager {
public:
	ResourceManager() = default;
	ResourceManager(const ResourceManager&) = delete;
	const ResourceManager& operator=(const ResourceManager&) = delete;

	// Returns resource id.
	// Note: uploadBuffer has to be kept alive after the above function calls because
	// the command list has not been executed yet that performs the actual copy.
	// The caller can Release the uploadBuffer after it knows the copy has been executed.
	size_t CreateDefaultBuffer(
		ID3D12Device& device,
		ID3D12GraphicsCommandList& cmdList,
		const void* initData,
		const uint64_t byteSize,
		Microsoft::WRL::ComPtr<ID3D12Resource>& defaultBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

	// Asserts if resource id is not present
	Microsoft::WRL::ComPtr<ID3D12Resource> GetResource(const size_t id);

	// This will invalidate all ids.
	void Clear() { mResources.resize(0); }

private:
	// Each index in the vector is the resource id
	typedef std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> Resources;
	Resources mResources;
};

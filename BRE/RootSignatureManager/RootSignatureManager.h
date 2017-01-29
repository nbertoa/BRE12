#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb\concurrent_hash_map.h>
#include <wrl.h>

// This class is responsible to create/get/erase root signatures
class RootSignatureManager {
public:
	RootSignatureManager() = delete;
	~RootSignatureManager() = delete;
	RootSignatureManager(const RootSignatureManager&) = delete;
	const RootSignatureManager& operator=(const RootSignatureManager&) = delete;
	RootSignatureManager(RootSignatureManager&&) = delete;
	RootSignatureManager& operator=(RootSignatureManager&&) = delete;

	static std::size_t CreateRootSignatureFromBlob(
		ID3DBlob& blob, 
		ID3D12RootSignature* &rootSignature) noexcept;

	// Preconditions:
	// "id" must be valid
	static ID3D12RootSignature& GetRootSignature(const std::size_t id) noexcept;

private:
	using RootSignatureById = tbb::concurrent_hash_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12RootSignature>>;
	static RootSignatureById mRootSignatureById;

	static std::mutex mMutex;
};

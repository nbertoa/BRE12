#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb\concurrent_hash_map.h>
#include <wrl.h>

// This class is responsible to create/get/erase root signatures
class RootSignatureManager {
public:
	static RootSignatureManager& Create(ID3D12Device& device) noexcept;
	static RootSignatureManager& Get() noexcept;
	
	~RootSignatureManager() = default;
	RootSignatureManager(const RootSignatureManager&) = delete;
	const RootSignatureManager& operator=(const RootSignatureManager&) = delete;
	RootSignatureManager(RootSignatureManager&&) = delete;
	RootSignatureManager& operator=(RootSignatureManager&&) = delete;

	std::size_t CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& desc, ID3D12RootSignature* &rootSign) noexcept;
	std::size_t CreateRootSignature(const D3D12_SHADER_BYTECODE& shaderByteCode, ID3D12RootSignature* &rootSign) noexcept;
	std::size_t CreateRootSignature(ID3DBlob& rootSignBlob, ID3D12RootSignature* &rootSign) noexcept;

	// Asserts id was not already registered
	ID3D12RootSignature& GetRootSignature(const std::size_t id) noexcept;

	// Asserts if id is not present
	void Erase(const std::size_t id) noexcept;

	// This will invalidate all ids.
	__forceinline void Clear() noexcept { mRootSignatureById.clear(); }

private:
	explicit RootSignatureManager(ID3D12Device& device);

	ID3D12Device& mDevice;

	using RootSignatureById = tbb::concurrent_hash_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12RootSignature>>;
	RootSignatureById mRootSignatureById;

	std::mutex mMutex;
};

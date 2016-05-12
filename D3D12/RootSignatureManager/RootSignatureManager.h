#pragma once

#include <d3d12.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <wrl.h>

#include <DXUtils\d3dx12.h>

class RootSignatureManager {
public:
	static std::unique_ptr<RootSignatureManager> gManager;

	RootSignatureManager(Microsoft::WRL::ComPtr<ID3D12Device>& device) : mDevice(device) {}
	RootSignatureManager(const RootSignatureManager&) = delete;
	const RootSignatureManager& operator=(const RootSignatureManager&) = delete;

	// Asserts if name was already registered
	std::size_t CreateRootSignature(const std::string& name, const CD3DX12_ROOT_SIGNATURE_DESC& desc, Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSign) noexcept;

	// Asserts id was not already registered
	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSignature(const std::size_t id) noexcept;

	// Asserts if id is not present
	void Erase(const std::size_t id) noexcept;

	// This will invalidate all ids.
	void Clear() noexcept { mRootSignatureById.clear(); }

private:
	Microsoft::WRL::ComPtr<ID3D12Device>& mDevice;

	using IdAndRootSignature = std::pair<std::size_t, Microsoft::WRL::ComPtr<ID3D12RootSignature>>;
	using RootSignatureById = std::unordered_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12RootSignature>>;
	RootSignatureById mRootSignatureById;
	std::hash<std::string> mHash;
};

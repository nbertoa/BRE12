#include "RootSignatureManager.h"

#include <Utils/DebugUtils.h>

std::unique_ptr<RootSignatureManager> RootSignatureManager::gManager = nullptr;

std::size_t RootSignatureManager::CreateRootSignature(const std::string& name, const CD3DX12_ROOT_SIGNATURE_DESC& desc, Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSign) noexcept {
	ASSERT(!name.empty());

	const std::size_t id{ mHash(name) };
	ASSERT(mRootSignatureById.find(id) == mRootSignatureById.end());

	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	CHECK_HR(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf()));
	CHECK_HR(mDevice->CreateRootSignature(0U, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(rootSign.GetAddressOf())));
	mRootSignatureById.insert(IdAndRootSignature{ id, rootSign });

	return id;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignatureManager::GetRootSignature(const std::size_t id) noexcept {
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
	RootSignatureById::iterator it{ mRootSignatureById.find(id) };
	ASSERT(it != mRootSignatureById.end());

	return it->second;
}

void RootSignatureManager::Erase(const std::size_t id) noexcept {
	ASSERT(mRootSignatureById.find(id) != mRootSignatureById.end());
	mRootSignatureById.erase(id);
}
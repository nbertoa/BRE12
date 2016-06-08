#include "RootSignatureManager.h"

#include <Utils/DebugUtils.h>

std::unique_ptr<RootSignatureManager> RootSignatureManager::gManager = nullptr;

std::size_t RootSignatureManager::CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& desc, ID3D12RootSignature* &rootSign) noexcept {
	const std::size_t id{ mRandGen.RandomNumber() };

	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

	mMutex.lock();
	CHECK_HR(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf()));
	mMutex.unlock();

	RootSignatureById::accessor accessor;
#ifdef _DEBUG
	mRootSignatureById.find(accessor, id);
	ASSERT(accessor.empty());
#endif

	mRootSignatureById.insert(accessor, id);
	Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSignature = accessor->second;

	mMutex.lock();
	CHECK_HR(mDevice.CreateRootSignature(0U, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(rootSignature.GetAddressOf())));
	mMutex.unlock();
	
	rootSign = rootSignature.Get();
	accessor.release();

	return id;
}

ID3D12RootSignature& RootSignatureManager::GetRootSignature(const std::size_t id) noexcept {
	RootSignatureById::accessor accessor;
	mRootSignatureById.find(accessor, id);
	ASSERT(!accessor.empty());
	ID3D12RootSignature* rootSign{ accessor->second.Get() };
	accessor.release();

	return *rootSign;
}

void RootSignatureManager::Erase(const std::size_t id) noexcept {
	RootSignatureById::accessor accessor;
	mRootSignatureById.find(accessor, id);
	ASSERT(!accessor.empty());
	mRootSignatureById.erase(accessor);
	accessor.release();
}
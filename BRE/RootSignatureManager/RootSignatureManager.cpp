#include "RootSignatureManager.h"

#include <D3Dcompiler.h>
#include <memory>

#include <DirectXManager/DirectXManager.h>
#include <Utils/DebugUtils.h>
#include <Utils/NumberGeneration.h>

RootSignatureManager::RootSignatureById RootSignatureManager::mRootSignatureById;
std::mutex RootSignatureManager::mMutex;

std::size_t RootSignatureManager::CreateRootSignatureFromBlob(
	ID3DBlob& blob, 
	ID3D12RootSignature* &rootSignature) noexcept 
{
	mMutex.lock();
	DirectXManager::GetDevice().CreateRootSignature(
		0U, 
		blob.GetBufferPointer(), 
		blob.GetBufferSize(), 
		IID_PPV_ARGS(&rootSignature));
	mMutex.unlock();

	const std::size_t id{ NumberGeneration::GetIncrementalSizeT() };
	RootSignatureById::accessor accessor;
#ifdef _DEBUG
	mRootSignatureById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mRootSignatureById.insert(accessor, id);
	accessor->second = Microsoft::WRL::ComPtr<ID3D12RootSignature>(rootSignature);
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
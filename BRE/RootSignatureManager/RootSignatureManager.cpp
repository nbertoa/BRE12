#include "RootSignatureManager.h"

#include <D3Dcompiler.h>

#include <DirectXManager/DirectXManager.h>
#include <Utils/DebugUtils.h>

namespace BRE {
RootSignatureManager::RootSignatures RootSignatureManager::mRootSignatures;
std::mutex RootSignatureManager::mMutex;

void
RootSignatureManager::EraseAll() noexcept
{
    for (ID3D12RootSignature* rootSignature : mRootSignatures) {
        BRE_ASSERT(rootSignature != nullptr);
        rootSignature->Release();
    }
}

ID3D12RootSignature&
RootSignatureManager::CreateRootSignatureFromBlob(ID3DBlob& blob) noexcept
{
    ID3D12RootSignature* rootSignature{ nullptr };

    mMutex.lock();
    DirectXManager::GetDevice().CreateRootSignature(0U,
                                                    blob.GetBufferPointer(),
                                                    blob.GetBufferSize(),
                                                    IID_PPV_ARGS(&rootSignature));
    mMutex.unlock();

    BRE_ASSERT(rootSignature != nullptr);
    mRootSignatures.insert(rootSignature);

    return *rootSignature;
}
}


#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb\concurrent_unordered_set.h>

// This class is responsible to create/get/erase root signatures
class RootSignatureManager {
public:
    RootSignatureManager() = delete;
    ~RootSignatureManager() = delete;
    RootSignatureManager(const RootSignatureManager&) = delete;
    const RootSignatureManager& operator=(const RootSignatureManager&) = delete;
    RootSignatureManager(RootSignatureManager&&) = delete;
    RootSignatureManager& operator=(RootSignatureManager&&) = delete;

    static void EraseAll() noexcept;

    static ID3D12RootSignature& CreateRootSignatureFromBlob(ID3DBlob& blob) noexcept;

private:
    using RootSignatures = tbb::concurrent_unordered_set<ID3D12RootSignature*>;
    static RootSignatures mRootSignatures;

    static std::mutex mMutex;
};

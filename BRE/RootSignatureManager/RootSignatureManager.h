#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb\concurrent_unordered_set.h>

namespace BRE {
///
/// @brief Responsible to create root signatures
///
class RootSignatureManager {
public:
    RootSignatureManager() = delete;
    ~RootSignatureManager() = delete;
    RootSignatureManager(const RootSignatureManager&) = delete;
    const RootSignatureManager& operator=(const RootSignatureManager&) = delete;
    RootSignatureManager(RootSignatureManager&&) = delete;
    RootSignatureManager& operator=(RootSignatureManager&&) = delete;

    ///
    /// @brief Releases all root signatures
    ///
    static void Clear() noexcept;

    ///
    /// @brief Create root signature from blob
    /// @param blob Blob
    /// @return Root signature
    ///
    static ID3D12RootSignature& CreateRootSignatureFromBlob(ID3DBlob& blob) noexcept;

private:
    static tbb::concurrent_unordered_set<ID3D12RootSignature*> mRootSignatures;

    static std::mutex mMutex;
};
}
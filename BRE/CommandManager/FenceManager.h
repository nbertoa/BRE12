#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb\concurrent_unordered_set.h>

namespace BRE {
///
/// Responsible for fences creation.
///
class FenceManager {
public:
    FenceManager() = delete;
    ~FenceManager() = delete;
    FenceManager(const FenceManager&) = delete;
    const FenceManager& operator=(const FenceManager&) = delete;
    FenceManager(FenceManager&&) = delete;
    FenceManager& operator=(FenceManager&&) = delete;

    ///
    /// @brief Release all the fences
    ///
    static void Clear() noexcept;

    ///
    /// @brief Create a fence
    /// @param fenceInitialValue Initial value at fence creation
    /// @param flags Initial fence flags
    /// @return The created fence
    ///
    static ID3D12Fence& CreateFence(const std::uint64_t fenceInitialValue,
                                    const D3D12_FENCE_FLAGS& flags) noexcept;

private:
    static tbb::concurrent_unordered_set<ID3D12Fence*> mFences;

    static std::mutex mMutex;
};
}



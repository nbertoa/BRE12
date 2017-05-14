#pragma once

#include <d3d12.h>
#include <D3Dcommon.h>
#include <mutex>
#include <tbb\concurrent_unordered_set.h>

namespace BRE {
///
/// @brief Responsible to load and handle shaders
///
class ShaderManager {
public:
    ShaderManager() = delete;
    ~ShaderManager() = delete;
    ShaderManager(const ShaderManager&) = delete;
    const ShaderManager& operator=(const ShaderManager&) = delete;
    ShaderManager(ShaderManager&&) = delete;
    ShaderManager& operator=(ShaderManager&&) = delete;

    ///
    /// @brief Releases all shaders
    ///
    static void Clear() noexcept;

    ///
    /// @brief Load shader file and get blob
    /// @param filename Filename. Must not be nullptr
    /// @return Loaded blob
    ///
    static ID3DBlob& LoadShaderFileAndGetBlob(const char* filename) noexcept;

    ///
    /// @brief Load shader file and get byte code
    /// @param filename Filename. Must not be nullptr
    /// @return Loaded byte code
    ///
    static D3D12_SHADER_BYTECODE LoadShaderFileAndGetBytecode(const char* filename) noexcept;

private:
    static tbb::concurrent_unordered_set<ID3DBlob*> mShaderBlobs;

    static std::mutex mMutex;
};

}


#pragma once

#include <d3d12.h>
#include <D3Dcommon.h>
#include <mutex>
#include <tbb\concurrent_unordered_set.h>

namespace BRE {
// Too load/get shaders
class ShaderManager {
public:
    ShaderManager() = delete;
    ~ShaderManager() = delete;
    ShaderManager(const ShaderManager&) = delete;
    const ShaderManager& operator=(const ShaderManager&) = delete;
    ShaderManager(ShaderManager&&) = delete;
    ShaderManager& operator=(ShaderManager&&) = delete;

    static void EraseAll() noexcept;

    // Preconditions:
    // - "filename" must not be nullptr
    static ID3DBlob& LoadShaderFileAndGetBlob(const char* filename) noexcept;
    static D3D12_SHADER_BYTECODE LoadShaderFileAndGetBytecode(const char* filename) noexcept;

private:
    using ShaderBlobs = tbb::concurrent_unordered_set<ID3DBlob*>;
    static ShaderBlobs mShaderBlobs;

    static std::mutex mMutex;
};

}


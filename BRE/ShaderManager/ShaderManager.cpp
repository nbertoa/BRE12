#include "ShaderManager.h"

#include <cstdint>
#include <D3Dcompiler.h>
#include <fstream>

#include <Utils/DebugUtils.h>

namespace BRE {
namespace {
///
/// @brief Load a blob
/// @param filename Filename. Must not be nullptr
/// @return Loaded blob
///
ID3DBlob*
LoadBlob(const std::string& filename) noexcept
{
    std::ifstream fileStream{ filename, std::ios::binary };
    BRE_ASSERT(fileStream);

    fileStream.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size{ static_cast<std::int32_t>(fileStream.tellg()) };
    fileStream.seekg(0, std::ios_base::beg);

    ID3DBlob* blob;
    BRE_CHECK_HR(D3DCreateBlob(size, &blob));

    fileStream.read(reinterpret_cast<char*>(blob->GetBufferPointer()), size);
    fileStream.close();

    return blob;
}
}

tbb::concurrent_unordered_set<ID3DBlob*> ShaderManager::mShaderBlobs;
std::mutex ShaderManager::mMutex;

void
ShaderManager::Clear() noexcept
{
    for (ID3DBlob* blob : mShaderBlobs) {
        BRE_ASSERT(blob != nullptr);
        blob->Release();
    }

    mShaderBlobs.clear();
}

ID3DBlob&
ShaderManager::LoadShaderFileAndGetBlob(const char* filename) noexcept
{
    BRE_ASSERT(filename != nullptr);

    ID3DBlob* blob{ nullptr };

    mMutex.lock();
    blob = LoadBlob(filename);
    mMutex.unlock();

    BRE_ASSERT(blob != nullptr);
    mShaderBlobs.insert(blob);

    return *blob;
}

D3D12_SHADER_BYTECODE
ShaderManager::LoadShaderFileAndGetBytecode(const char* filename) noexcept
{
    BRE_ASSERT(filename != nullptr);

    ID3DBlob* blob{ nullptr };

    mMutex.lock();
    blob = LoadBlob(filename);
    mMutex.unlock();

    BRE_ASSERT(blob != nullptr);
    mShaderBlobs.insert(blob);

    D3D12_SHADER_BYTECODE shaderByteCode
    {
        reinterpret_cast<uint8_t*>(blob->GetBufferPointer()),
        blob->GetBufferSize()
    };

    return shaderByteCode;
}
}
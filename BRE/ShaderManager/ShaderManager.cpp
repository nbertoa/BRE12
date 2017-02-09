#include "ShaderManager.h"

#include <cstdint>
#include <D3Dcompiler.h>
#include <fstream>

#include <Utils/DebugUtils.h>

namespace {
	ID3DBlob* LoadBlob(const std::string& filename) noexcept {
		std::ifstream fileStream{ filename, std::ios::binary };
		ASSERT(fileStream);

		fileStream.seekg(0, std::ios_base::end);
		std::ifstream::pos_type size{ static_cast<std::int32_t>(fileStream.tellg()) };
		fileStream.seekg(0, std::ios_base::beg);

		ID3DBlob* blob;
		CHECK_HR(D3DCreateBlob(size, &blob));

		fileStream.read(reinterpret_cast<char*>(blob->GetBufferPointer()), size);
		fileStream.close();

		return blob;
	}
}

ShaderManager::ShaderBlobs ShaderManager::mShaderBlobs;
std::mutex ShaderManager::mMutex;

void ShaderManager::EraseAll() noexcept {
	for (ID3DBlob* blob : mShaderBlobs) {
		ASSERT(blob != nullptr);
		blob->Release();
	}
}

ID3DBlob& ShaderManager::LoadShaderFileAndGetBlob(const char* filename) noexcept {
	ASSERT(filename != nullptr);
	
	ID3DBlob* blob{ nullptr };

	mMutex.lock();
	blob = LoadBlob(filename);
	mMutex.unlock();

	ASSERT(blob != nullptr);
	mShaderBlobs.insert(blob);

	return *blob;
}

D3D12_SHADER_BYTECODE ShaderManager::LoadShaderFileAndGetBytecode(const char* filename) noexcept {
	ASSERT(filename != nullptr);

	ID3DBlob* blob{ nullptr };

	mMutex.lock();
	blob = LoadBlob(filename);
	mMutex.unlock();

	ASSERT(blob != nullptr);
	mShaderBlobs.insert(blob);

	D3D12_SHADER_BYTECODE shaderByteCode
	{
		reinterpret_cast<uint8_t*>(blob->GetBufferPointer()),
		blob->GetBufferSize()
	};

	return shaderByteCode;
}
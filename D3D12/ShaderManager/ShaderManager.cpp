#include "ShaderManager.h"

#include <cstdint>
#include <D3Dcompiler.h>
#include <fstream>
#include <memory>

#include <Utils/DebugUtils.h>
#include <Utils/NumberGeneration.h>

namespace {
	ID3DBlob* LoadBlob(const std::string& filename) noexcept {
		ASSERT(!filename.empty());

		std::ifstream fin{ filename, std::ios::binary };
		ASSERT(fin);

		fin.seekg(0, std::ios_base::end);
		std::ifstream::pos_type size{ static_cast<std::int32_t>(fin.tellg()) };
		fin.seekg(0, std::ios_base::beg);

		ID3DBlob* blob;
		CHECK_HR(D3DCreateBlob(size, &blob));

		fin.read(reinterpret_cast<char*>(blob->GetBufferPointer()), size);
		fin.close();

		return blob;
	}

	std::unique_ptr<ShaderManager> gManager{ nullptr };
}

ShaderManager& ShaderManager::Create() noexcept {
	ASSERT(gManager == nullptr);
	gManager.reset(new ShaderManager());
	return *gManager.get();
}
ShaderManager& ShaderManager::Get() noexcept {
	ASSERT(gManager != nullptr);
	return *gManager.get();
}

std::size_t ShaderManager::LoadShaderFile(const char* filename, ID3DBlob* &blob) noexcept {
	ASSERT(filename != nullptr);
	
	mMutex.lock();
	blob = LoadBlob(filename);
	mMutex.unlock();

	const std::size_t id{ NumberGeneration::IncrementalSizeT() };
	BlobById::accessor accessor;
#ifdef _DEBUG
	mBlobById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mBlobById.insert(accessor, id);
	accessor->second = Microsoft::WRL::ComPtr<ID3DBlob>(blob);
	accessor.release();

	return id;
}

std::size_t ShaderManager::LoadShaderFile(const char* filename, D3D12_SHADER_BYTECODE& shaderByteCode) noexcept {
	ASSERT(filename != nullptr);

	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	mMutex.lock();
	blob = LoadBlob(filename);
	mMutex.unlock();

	const std::size_t id{ NumberGeneration::IncrementalSizeT() };
	BlobById::accessor accessor;
#ifdef _DEBUG
	mBlobById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mBlobById.insert(accessor, id);
	accessor->second = blob;
	accessor.release();

	ASSERT(blob.Get());
	shaderByteCode.pShaderBytecode = reinterpret_cast<uint8_t*>(blob->GetBufferPointer());
	shaderByteCode.BytecodeLength = blob->GetBufferSize();

	return id;
}

ID3DBlob& ShaderManager::GetBlob(const std::size_t id) noexcept {
	BlobById::accessor accessor;
	mBlobById.find(accessor, id);
	ASSERT(!accessor.empty());

	ID3DBlob* blob{ accessor->second.Get() };
	accessor.release();

	return *blob;
}

D3D12_SHADER_BYTECODE ShaderManager::GetShaderByteCode(const std::size_t id) noexcept {
	BlobById::accessor accessor;
	mBlobById.find(accessor, id);
	ASSERT(!accessor.empty());
	ID3DBlob* blob{ accessor->second.Get() };
	accessor.release();
	
	D3D12_SHADER_BYTECODE shaderByteCode{};
	shaderByteCode.pShaderBytecode = reinterpret_cast<uint8_t*>(blob->GetBufferPointer());
	shaderByteCode.BytecodeLength = blob->GetBufferSize();

	return shaderByteCode;
}

void ShaderManager::Erase(const std::size_t id) noexcept {
	BlobById::accessor accessor;
	mBlobById.find(accessor, id);
	ASSERT(!accessor.empty());
	mBlobById.erase(accessor);
	accessor.release();
}
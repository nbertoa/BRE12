#include "ShaderManager.h"

#include <cstdint>
#include <D3Dcompiler.h>
#include <fstream>

#include <Utils/DebugUtils.h>

namespace {
	Microsoft::WRL::ComPtr<ID3DBlob> LoadBlob(const std::string& filename) {
		ASSERT(!filename.empty());

		std::ifstream fin{ filename, std::ios::binary };
		ASSERT(fin);

		fin.seekg(0, std::ios_base::end);
		std::ifstream::pos_type size{ (int)fin.tellg() };
		fin.seekg(0, std::ios_base::beg);

		Microsoft::WRL::ComPtr<ID3DBlob> blob;
		CHECK_HR(D3DCreateBlob(size, blob.GetAddressOf()));

		fin.read((char*)blob->GetBufferPointer(), size);
		fin.close();

		return blob;
	}
}


size_t ShaderManager::LoadShaderFile(const std::string& filename, Microsoft::WRL::ComPtr<ID3DBlob>& blob) {
	ASSERT(!filename.empty());

	const size_t id{ mHash(filename) };
	BlobById::iterator it{ mBlobById.find(id) };
	if (it != mBlobById.end()) {
		blob = it->second;
	} 
	else {
		blob = LoadBlob(filename);
		mBlobById.insert(IdAndBlob(id, blob));
	}

	ASSERT(blob.Get());

	return id;
}

size_t ShaderManager::LoadShaderFile(const std::string& filename, D3D12_SHADER_BYTECODE& shaderByteCode) {
	ASSERT(!filename.empty());

	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	const size_t id{ mHash(filename) };
	BlobById::iterator it{ mBlobById.find(id) };
	if (it != mBlobById.end()) {
		blob = it->second;
	}
	else {
		blob = LoadBlob(filename);
		mBlobById.insert(IdAndBlob(id, blob));
	}

	ASSERT(blob.Get());
	shaderByteCode.pShaderBytecode = (uint8_t*)blob->GetBufferPointer();
	shaderByteCode.BytecodeLength = blob->GetBufferSize();

	return id;
}

std::unique_ptr<ShaderManager> ShaderManager::gShaderMgr = nullptr;

Microsoft::WRL::ComPtr<ID3DBlob> ShaderManager::GetBlob(const size_t id) {
	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	BlobById::iterator it{ mBlobById.find(id) };
	ASSERT(it != mBlobById.end());

	return it->second;
}

D3D12_SHADER_BYTECODE ShaderManager::GetShaderByteCode(const size_t id) {
	BlobById::iterator it{ mBlobById.find(id) };
	ASSERT(it != mBlobById.end());
	D3D12_SHADER_BYTECODE shaderByteCode{};
	shaderByteCode.pShaderBytecode = (uint8_t*)it->second->GetBufferPointer();
	shaderByteCode.BytecodeLength = it->second->GetBufferSize();

	return shaderByteCode;
}

void ShaderManager::Erase(const size_t id) {
	ASSERT(mBlobById.find(id) != mBlobById.end());
	mBlobById.erase(id);
}
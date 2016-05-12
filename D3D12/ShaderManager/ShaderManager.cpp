#include "ShaderManager.h"

#include <cstdint>
#include <D3Dcompiler.h>
#include <fstream>

#include <Utils/DebugUtils.h>

namespace {
	Microsoft::WRL::ComPtr<ID3DBlob> LoadBlob(const std::string& filename) noexcept {
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

std::unique_ptr<ShaderManager> ShaderManager::gManager = nullptr;

std::size_t ShaderManager::LoadShaderFile(const std::string& filename, Microsoft::WRL::ComPtr<ID3DBlob>& blob) noexcept {
	ASSERT(!filename.empty());

	const std::size_t id{ mHash(filename) };
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

std::size_t ShaderManager::LoadShaderFile(const std::string& filename, D3D12_SHADER_BYTECODE& shaderByteCode) noexcept {
	ASSERT(!filename.empty());

	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	const std::size_t id{ mHash(filename) };
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

std::size_t ShaderManager::AddInputLayout(const std::string& name, const std::vector<D3D12_INPUT_ELEMENT_DESC>& desc) noexcept {
	ASSERT(!name.empty());

	const std::size_t id{ mHash(name) };
	ASSERT(mInputLayoutById.find(id) == mInputLayoutById.end());
	mInputLayoutById.insert(IdAndInputLayout{ id, desc });

	return id;
}

Microsoft::WRL::ComPtr<ID3DBlob> ShaderManager::GetBlob(const std::size_t id) noexcept {
	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	BlobById::iterator it{ mBlobById.find(id) };
	ASSERT(it != mBlobById.end());

	return it->second;
}

D3D12_SHADER_BYTECODE ShaderManager::GetShaderByteCode(const std::size_t id) noexcept {
	BlobById::iterator it{ mBlobById.find(id) };
	ASSERT(it != mBlobById.end());
	D3D12_SHADER_BYTECODE shaderByteCode{};
	shaderByteCode.pShaderBytecode = (uint8_t*)it->second->GetBufferPointer();
	shaderByteCode.BytecodeLength = it->second->GetBufferSize();

	return shaderByteCode;
}

const std::vector<D3D12_INPUT_ELEMENT_DESC>& ShaderManager::GetInputLayout(const std::size_t id) noexcept {
	InputLayoutById::iterator it{ mInputLayoutById.find(id) };
	ASSERT(it != mInputLayoutById.end());

	return it->second;
}

void ShaderManager::EraseShader(const std::size_t id) noexcept {
	ASSERT(mBlobById.find(id) != mBlobById.end());
	mBlobById.erase(id);
}

void ShaderManager::EraseInputLayout(const std::size_t id) noexcept {
	ASSERT(mInputLayoutById.find(id) != mInputLayoutById.end());
	mInputLayoutById.erase(id);
}
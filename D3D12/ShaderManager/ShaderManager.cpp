#include "ShaderManager.h"

#include <cstdint>
#include <D3Dcompiler.h>
#include <fstream>

#include <Utils/DebugUtils.h>
#include <Utils/HashUtils.h>

namespace {
	ID3DBlob* LoadBlob(const std::string& filename) noexcept {
		ASSERT(!filename.empty());

		std::ifstream fin{ filename, std::ios::binary };
		ASSERT(fin);

		fin.seekg(0, std::ios_base::end);
		std::ifstream::pos_type size{ (int)fin.tellg() };
		fin.seekg(0, std::ios_base::beg);

		ID3DBlob* blob;
		CHECK_HR(D3DCreateBlob(size, &blob));

		fin.read((char*)blob->GetBufferPointer(), size);
		fin.close();

		return blob;
	}
}

std::unique_ptr<ShaderManager> ShaderManager::gManager = nullptr;

std::size_t ShaderManager::LoadShaderFile(const char* filename, ID3DBlob* &blob) noexcept {
	ASSERT(filename != nullptr);

	const std::size_t id{ HashUtils::HashCString(filename) };

	BlobById::accessor accessor;
	mBlobById.find(accessor,id);

	if (!accessor.empty()) {
		blob = accessor->second.Get();
	} 
	else {
		blob = LoadBlob(filename);
		mBlobById.insert(accessor, id);
		accessor->second = Microsoft::WRL::ComPtr<ID3DBlob>(blob);
	}
	accessor.release();

	return id;
}

std::size_t ShaderManager::LoadShaderFile(const char* filename, D3D12_SHADER_BYTECODE& shaderByteCode) noexcept {
	ASSERT(filename != nullptr);

	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	const std::size_t id{ HashUtils::HashCString(filename) };

	BlobById::accessor accessor;
	mBlobById.find(accessor, id);
	if (!accessor.empty()) {
		blob = accessor->second;
	}
	else {
		blob = LoadBlob(filename);
		mBlobById.insert(accessor, id);
		accessor->second = blob;
	}
	accessor.release();

	ASSERT(blob.Get());
	shaderByteCode.pShaderBytecode = (uint8_t*)blob->GetBufferPointer();
	shaderByteCode.BytecodeLength = blob->GetBufferSize();

	return id;
}

std::size_t ShaderManager::AddInputLayout(const char* name, const std::vector<D3D12_INPUT_ELEMENT_DESC>& desc) noexcept {
	ASSERT(name != nullptr);

	const std::size_t id{ HashUtils::HashCString(name) };
	InputLayoutById::accessor accesor;
#ifdef _DEBUG
	mInputLayoutById.find(accesor, id);
	ASSERT(accesor.empty());
#endif
	mInputLayoutById.insert(accesor, id);
	accesor->second = desc;
	accesor.release();

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
	shaderByteCode.pShaderBytecode = (uint8_t*)blob->GetBufferPointer();
	shaderByteCode.BytecodeLength = blob->GetBufferSize();

	return shaderByteCode;
}

const std::vector<D3D12_INPUT_ELEMENT_DESC>& ShaderManager::GetInputLayout(const std::size_t id) noexcept {
	InputLayoutById::accessor accessor;
	mInputLayoutById.find(accessor, id);
	ASSERT(accessor.empty());
	const std::vector<D3D12_INPUT_ELEMENT_DESC>& desc{ accessor->second };
	accessor.release();
	
	return desc;
}

void ShaderManager::EraseShader(const std::size_t id) noexcept {
	BlobById::accessor accessor;
	mBlobById.find(accessor, id);
	ASSERT(!accessor.empty());
	mBlobById.erase(accessor);
	accessor.release();
}

void ShaderManager::EraseInputLayout(const std::size_t id) noexcept {
	InputLayoutById::accessor accessor;
	mInputLayoutById.find(accessor, id);
	ASSERT(!accessor.empty());
	mInputLayoutById.erase(accessor);
	accessor.release();
}
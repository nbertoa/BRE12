#include "ShaderManager.h"

#include <DXUtils/D3dUtils.h>
#include <Utils/DebugUtils.h>

size_t ShaderManager::LoadShaderFile(const std::string& filename, Microsoft::WRL::ComPtr<ID3DBlob>& blob) {
	const size_t id = mHash(filename);
	BlobById::iterator it = mBlobById.find(id);
	if (it != mBlobById.end()) {
		blob = it->second;
	} 
	else {
		blob = D3dUtils::LoadBlob(filename);
		mBlobById.insert(IdAndBlob(id, blob));
	}

	ASSERT(blob.Get());

	return id;
}

size_t ShaderManager::LoadShaderFile(const std::string& filename, D3D12_SHADER_BYTECODE& shaderByteCode) {
	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	const size_t id = mHash(filename);
	BlobById::iterator it = mBlobById.find(id);
	if (it != mBlobById.end()) {
		blob = it->second;
	}
	else {
		blob = D3dUtils::LoadBlob(filename);
		mBlobById.insert(IdAndBlob(id, blob));
	}

	ASSERT(blob.Get());
	shaderByteCode.pShaderBytecode = (uint8_t*)blob->GetBufferPointer();
	shaderByteCode.BytecodeLength = blob->GetBufferSize();

	return id;
}

Microsoft::WRL::ComPtr<ID3DBlob> ShaderManager::GetBlob(const size_t id) {
	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	BlobById::iterator it = mBlobById.find(id);
	if (it != mBlobById.end()) {
		blob =  it->second;
	}

	return blob;
}

D3D12_SHADER_BYTECODE ShaderManager::GetShaderByteCode(const size_t id) {
	D3D12_SHADER_BYTECODE shaderByteCode = {};
	BlobById::iterator it = mBlobById.find(id);
	if (it != mBlobById.end()) {
		shaderByteCode.pShaderBytecode = (uint8_t*)it->second->GetBufferPointer();
		shaderByteCode.BytecodeLength = it->second->GetBufferSize();
	}

	return shaderByteCode;
}
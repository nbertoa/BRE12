#pragma once

#include <cstdint>
#include <d3d12.h>
#include <D3Dcommon.h>
#include <functional>
#include <string>
#include <unordered_map>
#include <wrl.h>

class ShaderManager {
public:
	ShaderManager() = default;
	ShaderManager(const ShaderManager&) = delete;
	const ShaderManager& operator=(const ShaderManager&) = delete;

	size_t LoadShaderFile(const std::string& filename, Microsoft::WRL::ComPtr<ID3DBlob>& blob);
	size_t LoadShaderFile(const std::string& filename, D3D12_SHADER_BYTECODE& shaderByteCode);

	// If there is not a blob with passed id, then returned blob will be invalid
	Microsoft::WRL::ComPtr<ID3DBlob> GetBlob(const size_t id);

	// If there is not a blob with passed id, then returned D3D12_SHADER_BYTECODE will be invalid
	D3D12_SHADER_BYTECODE GetShaderByteCode(const size_t id);

private:
	typedef std::pair<size_t, Microsoft::WRL::ComPtr<ID3DBlob>> IdAndBlob;
	typedef std::unordered_map<size_t, Microsoft::WRL::ComPtr<ID3DBlob>> BlobById;
	BlobById mBlobById;
	std::hash<std::string> mHash;
};

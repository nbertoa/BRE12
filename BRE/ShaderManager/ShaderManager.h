#pragma once

#include <d3d12.h>
#include <D3Dcommon.h>
#include <mutex>
#include <tbb\concurrent_hash_map.h>
#include <wrl.h>

// Too create/get/erase shaders
class ShaderManager {
public:
	ShaderManager() = delete;
	~ShaderManager() = delete;
	ShaderManager(const ShaderManager&) = delete;
	const ShaderManager& operator=(const ShaderManager&) = delete;
	ShaderManager(ShaderManager&&) = delete;
	ShaderManager& operator=(ShaderManager&&) = delete;

	// Returns id to get blob/shader byte code after creation
	// Preconditions:
	// - "filename" must not be nullptr
	static std::size_t LoadShaderFile(const char* filename, ID3DBlob* &blob) noexcept;
	static std::size_t LoadShaderFile(const char* filename, D3D12_SHADER_BYTECODE& shaderByteCode) noexcept;
	
	// Preconditions:
	// - "id" must be valid.
	static ID3DBlob& GetBlob(const std::size_t id) noexcept;
	static D3D12_SHADER_BYTECODE GetShaderByteCode(const std::size_t id) noexcept;

private:
	using BlobById = tbb::concurrent_hash_map<std::size_t, Microsoft::WRL::ComPtr<ID3DBlob>>;
	static BlobById mBlobById;

	static std::mutex mMutex;
};

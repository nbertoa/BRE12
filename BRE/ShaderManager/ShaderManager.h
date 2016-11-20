#pragma once

#include <d3d12.h>
#include <D3Dcommon.h>
#include <mutex>
#include <tbb\concurrent_hash_map.h>
#include <wrl.h>

// This class is responsible to create/get/erase shaders
class ShaderManager {
public:
	static ShaderManager& Create() noexcept;
	static ShaderManager& Get() noexcept;
	
	~ShaderManager() = default;
	ShaderManager(const ShaderManager&) = delete;
	const ShaderManager& operator=(const ShaderManager&) = delete;
	ShaderManager(ShaderManager&&) = delete;
	ShaderManager& operator=(ShaderManager&&) = delete;

	// Returns id to get blob/shader byte code after creation
	std::size_t LoadShaderFile(const char* filename, ID3DBlob* &blob) noexcept;
	std::size_t LoadShaderFile(const char* filename, D3D12_SHADER_BYTECODE& shaderByteCode) noexcept;
	
	// Asserts if id does not exist
	ID3DBlob& GetBlob(const std::size_t id) noexcept;
	D3D12_SHADER_BYTECODE GetShaderByteCode(const std::size_t id) noexcept;

	// Asserts if id is not present
	void Erase(const std::size_t id) noexcept;

	// Invalidate all ids.
	__forceinline void Clear() noexcept { mBlobById.clear(); }

private:
	ShaderManager() = default;

	using BlobById = tbb::concurrent_hash_map<std::size_t, Microsoft::WRL::ComPtr<ID3DBlob>>;
	BlobById mBlobById;

	std::mutex mMutex;
};

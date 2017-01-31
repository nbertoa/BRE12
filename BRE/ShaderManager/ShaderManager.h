#pragma once

#include <d3d12.h>
#include <D3Dcommon.h>
#include <mutex>
#include <tbb\concurrent_unordered_set.h>

// Too create/get/erase shaders
class ShaderManager {
public:
	ShaderManager() = delete;
	~ShaderManager();
	ShaderManager(const ShaderManager&) = delete;
	const ShaderManager& operator=(const ShaderManager&) = delete;
	ShaderManager(ShaderManager&&) = delete;
	ShaderManager& operator=(ShaderManager&&) = delete;

	// Preconditions:
	// - "filename" must not be nullptr
	static ID3DBlob& LoadShaderFileAndGetBlob(const char* filename) noexcept;
	static D3D12_SHADER_BYTECODE LoadShaderFileAndGetBytecode(const char* filename) noexcept;

private:
	using ShaderBlobs = tbb::concurrent_unordered_set<ID3DBlob*>;
	static ShaderBlobs mShaderBlobs;

	static std::mutex mMutex;
};

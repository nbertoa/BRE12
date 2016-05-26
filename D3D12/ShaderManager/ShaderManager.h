#pragma once

#include <d3d12.h>
#include <D3Dcommon.h>
#include <memory>
#include <tbb\concurrent_hash_map.h>
#include <vector>
#include <wrl.h>

class ShaderManager {
public:
	static std::unique_ptr<ShaderManager> gManager;

	ShaderManager() = default;
	ShaderManager(const ShaderManager&) = delete;
	const ShaderManager& operator=(const ShaderManager&) = delete;

	// Returns id to get blob/shader byte code after creation
	std::size_t LoadShaderFile(const char* filename, ID3DBlob* &blob) noexcept;
	std::size_t LoadShaderFile(const char* filename, D3D12_SHADER_BYTECODE& shaderByteCode) noexcept;

	// Asserts if name was already registered.
	std::size_t AddInputLayout(const char* name, const std::vector<D3D12_INPUT_ELEMENT_DESC>& desc) noexcept;

	// Asserts if id does not exist
	ID3DBlob& GetBlob(const std::size_t id) noexcept;
	D3D12_SHADER_BYTECODE GetShaderByteCode(const std::size_t id) noexcept;
	const std::vector<D3D12_INPUT_ELEMENT_DESC>& GetInputLayout(const std::size_t id) noexcept;

	// Asserts if id is not present
	void EraseShader(const std::size_t id) noexcept;
	void EraseInputLayout(const std::size_t id) noexcept;

	// Invalidate all ids.
	void ClearShaders() noexcept { mBlobById.clear(); }
	void ClearInputLayouts() noexcept { mInputLayoutById.clear(); }
	void Clear() noexcept { ClearShaders(); ClearInputLayouts(); }

private:
	using BlobById = tbb::concurrent_hash_map<std::size_t, Microsoft::WRL::ComPtr<ID3DBlob>>;
	BlobById mBlobById;

	using InputLayoutById = tbb::concurrent_hash_map<std::size_t, std::vector<D3D12_INPUT_ELEMENT_DESC>>;
	InputLayoutById mInputLayoutById;
};

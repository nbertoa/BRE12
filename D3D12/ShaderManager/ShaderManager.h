#pragma once

#include <d3d12.h>
#include <D3Dcommon.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <wrl.h>

class ShaderManager {
public:
	static std::unique_ptr<ShaderManager> gShaderMgr;

	ShaderManager() = default;
	ShaderManager(const ShaderManager&) = delete;
	const ShaderManager& operator=(const ShaderManager&) = delete;

	// Returns id to get blob/shader byte code after creation
	std::size_t LoadShaderFile(const std::string& filename, Microsoft::WRL::ComPtr<ID3DBlob>& blob) noexcept;
	std::size_t LoadShaderFile(const std::string& filename, D3D12_SHADER_BYTECODE& shaderByteCode) noexcept;

	// Asserts if there is not a blob with current id
	Microsoft::WRL::ComPtr<ID3DBlob> GetBlob(const std::size_t id) noexcept;

	// Asserts if there is not a blob with current id
	D3D12_SHADER_BYTECODE GetShaderByteCode(const std::size_t id) noexcept;

	// Asserts if id is not present
	void Erase(const std::size_t id) noexcept;

	// This will invalidate all ids.
	void Clear() noexcept { mBlobById.clear(); }

private:
	using IdAndBlob = std::pair<std::size_t, Microsoft::WRL::ComPtr<ID3DBlob>>;
	using BlobById = std::unordered_map<std::size_t, Microsoft::WRL::ComPtr<ID3DBlob>>;
	BlobById mBlobById;
	std::hash<std::string> mHash;
};

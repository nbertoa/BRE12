#pragma once

#include <d3d12.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <wrl.h>

class PSOManager {
public:
	static std::unique_ptr<PSOManager> gPSOMgr;

	PSOManager(Microsoft::WRL::ComPtr<ID3D12Device>& device) : mDevice(device) {}
	PSOManager(const PSOManager&) = delete;
	const PSOManager& operator=(const PSOManager&) = delete;

	std::size_t CreateGraphicsPSO(const std::string& name, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, Microsoft::WRL::ComPtr<ID3D12PipelineState>& pso);

	// Asserts if there is not a valid ID3D12PipelineState with current id
	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPSO(const std::size_t id);

	// Asserts if id is not present
	void Erase(const std::size_t id);

	// This will invalidate all ids.
	void Clear() { mPSOById.clear(); }

private:
	Microsoft::WRL::ComPtr<ID3D12Device>& mDevice;

	using IdAndPSO = std::pair<std::size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>>;
	using PSOById = std::unordered_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>>;
	PSOById mPSOById;
	std::hash<std::string> mHash;
};

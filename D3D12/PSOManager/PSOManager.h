#pragma once

#include <d3d12.h>
#include <functional>
#include <string>
#include <unordered_map>
#include <wrl.h>

class PSOManager {
public:
	PSOManager(ID3D12Device& device) : mDevice(device) {}
	PSOManager(const PSOManager&) = delete;
	const PSOManager& operator=(const PSOManager&) = delete;

	size_t CreateGraphicsPSO(const std::string& name, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, Microsoft::WRL::ComPtr<ID3D12PipelineState>& pso);

	// Asserts if there is not a valid ID3D12PipelineState with current id
	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPSO(const size_t id);

	// Asserts if id is not present
	void Erase(const size_t id);

	// This will invalidate all ids.
	void Clear() { mPSOById.clear(); }

private:
	ID3D12Device& mDevice;

	typedef std::pair<size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>> IdAndPSO;
	typedef std::unordered_map<size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>> PSOById;
	PSOById mPSOById;
	std::hash<std::string> mHash;
};

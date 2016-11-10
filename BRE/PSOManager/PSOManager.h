#pragma once

#include <d3d12.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/mutex.h>
#include <wrl.h>

// This class is responsible to create/get/erase pipeline state objects
class PSOManager {
public:
	static PSOManager& Create(ID3D12Device& device) noexcept;
	static PSOManager& Get() noexcept;
		
	~PSOManager() = default;
	PSOManager(const PSOManager&) = delete;
	const PSOManager& operator=(const PSOManager&) = delete;
	PSOManager(PSOManager&&) = delete;
	PSOManager& operator=(PSOManager&&) = delete;

	std::size_t CreateGraphicsPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, ID3D12PipelineState* &pso) noexcept;

	// Asserts if there is not a valid ID3D12PipelineState with current id
	ID3D12PipelineState& GetPSO(const std::size_t id) noexcept;

	// Asserts if id is not present
	void Erase(const std::size_t id) noexcept;

	// This will invalidate all ids.
	__forceinline void Clear() noexcept { mPSOById.clear(); }

private:
	explicit PSOManager(ID3D12Device& device);

	ID3D12Device& mDevice;

	using PSOById = tbb::concurrent_hash_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>>;
	PSOById mPSOById;

	tbb::mutex mMutex;
};

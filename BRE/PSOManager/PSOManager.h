#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb/concurrent_hash_map.h>
#include <wrl.h>

#include <DXUtils/D3DFactory.h>

// To create/get/erase pipeline state objects
class PSOManager {
public:
	// Preconditions:
	// - Create() must be called once
	static PSOManager& Create() noexcept;

	// Preconditions:
	// - Create() must be called before this method
	static PSOManager& Get() noexcept;
		
	~PSOManager() = default;
	PSOManager(const PSOManager&) = delete;
	const PSOManager& operator=(const PSOManager&) = delete;
	PSOManager(PSOManager&&) = delete;
	PSOManager& operator=(PSOManager&&) = delete;

	struct PSOCreationData {
		PSOCreationData() = default;
		~PSOCreationData() = default;
		PSOCreationData(const PSOCreationData&) = delete;
		const PSOCreationData& operator=(const PSOCreationData&) = delete;
		PSOCreationData(PSOCreationData&&) = delete;
		PSOCreationData& operator=(PSOCreationData&&) = delete;

		bool IsDataValid() const noexcept;

		// If a shader filename is nullptr, then we do not load it.
		std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout{};
		const char* mRootSignFilename{ nullptr };
		const char* mVSFilename{ nullptr };
		const char* mGSFilename{ nullptr };
		const char* mDSFilename{ nullptr };
		const char* mHSFilename{ nullptr };
		const char* mPSFilename{ nullptr };

		D3D12_BLEND_DESC mBlendDesc = D3DFactory::GetDisabledBlendDesc();
		D3D12_RASTERIZER_DESC mRasterizerDesc = D3DFactory::GetDefaultRasterizerDesc();
		D3D12_DEPTH_STENCIL_DESC mDepthStencilDesc = D3DFactory::GetDefaultDepthStencilDesc();
		std::uint32_t mNumRenderTargets{ 0U };
		DXGI_FORMAT mRtFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{ DXGI_FORMAT_UNKNOWN };
		DXGI_SAMPLE_DESC mSampleDesc{ 1U, 0U };
		std::uint32_t mSampleMask{ UINT_MAX };
		D3D12_PRIMITIVE_TOPOLOGY_TYPE mTopology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
	};

	// Returns id of the pipeline state object
	// Preconditions:
	// - "psoData" must be valid
	std::size_t CreateGraphicsPSO(
		const PSOManager::PSOCreationData& psoData,
		ID3D12PipelineState* &pso,
		ID3D12RootSignature* &rootSign) noexcept;

	// Preconditions:
	// - "id" must be valid
	ID3D12PipelineState& GetGraphicsPSO(const std::size_t id) noexcept;

private:
	PSOManager() = default;

	std::size_t CreateGraphicsPSOByDescriptor(
		const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDescriptor, 
		ID3D12PipelineState* &pso) noexcept;

	using PSOById = tbb::concurrent_hash_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>>;
	PSOById mPSOById;

	std::mutex mMutex;
};

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


		std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayoutDescriptors{};

		ID3DBlob* mRootSignatureBlob{ nullptr };
		ID3D12RootSignature* mRootSignature{ nullptr };

		// If a shader bytecode is not valid, then we do not load it.
		D3D12_SHADER_BYTECODE mVertexShaderBytecode{ 0UL };
		D3D12_SHADER_BYTECODE mGeometryShaderBytecode{ 0UL };
		D3D12_SHADER_BYTECODE mDomainShaderBytecode{ 0UL };
		D3D12_SHADER_BYTECODE mHullShaderBytecode{ 0UL };
		D3D12_SHADER_BYTECODE mPixelShaderBytecode{ 0UL };

		D3D12_BLEND_DESC mBlendDescriptor = D3DFactory::GetDisabledBlendDesc();
		D3D12_RASTERIZER_DESC mRasterizerDescriptor = D3DFactory::GetDefaultRasterizerDesc();
		D3D12_DEPTH_STENCIL_DESC mDepthStencilDescriptor = D3DFactory::GetDefaultDepthStencilDesc();
		std::uint32_t mNumRenderTargets{ 0U };
		DXGI_FORMAT mRenderTargetFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{ DXGI_FORMAT_UNKNOWN };
		DXGI_SAMPLE_DESC mSampleDescriptor{ 1U, 0U };
		std::uint32_t mSampleMask{ UINT_MAX };
		D3D12_PRIMITIVE_TOPOLOGY_TYPE mPrimitiveTopologyType{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
	};

	// Returns id of the pipeline state object
	// Preconditions:
	// - "psoCreationData" must be valid
	std::size_t CreateGraphicsPSO(
		const PSOManager::PSOCreationData& psoCreationData,
		ID3D12PipelineState* &pso,
		ID3D12RootSignature* &rootSignature) noexcept;

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

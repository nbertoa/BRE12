#pragma once

#include <d3d12.h>
#include <vector>

#include <DXUtils/D3DFactory.h>

// Used to create Pipeline State Objects and Root Signatures (loaded from a shader file)
class PSOCreatorTask {
public:
	struct Input {
		Input() = default;

		bool ValidateData() const noexcept;

		// If a shader filename is nullptr, then we do not load it.
		std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout{};
		const char* mRootSignFilename{ nullptr };
		const char* mVSFilename{ nullptr };
		const char* mGSFilename{ nullptr };
		const char* mDSFilename{ nullptr };
		const char* mHSFilename{ nullptr };
		const char* mPSFilename{ nullptr };

		D3D12_BLEND_DESC mBlendDesc = D3DFactory::DefaultBlendDesc();
		D3D12_RASTERIZER_DESC mRasterizerDesc = D3DFactory::DefaultRasterizerDesc();
		D3D12_DEPTH_STENCIL_DESC mDepthStencilDesc = D3DFactory::DefaultDepthStencilDesc();
		std::uint32_t mNumRenderTargets{ 1U };
		DXGI_SAMPLE_DESC mSampleDesc{ 1U, 0U };
		std::uint32_t mSampleMask{ UINT_MAX };
		D3D12_PRIMITIVE_TOPOLOGY_TYPE mTopology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
	};

	struct Output {
		Output() = default;

		ID3D12PipelineState* mPSO{ nullptr };
		ID3D12RootSignature* mRootSign{ nullptr };
	};

	PSOCreatorTask(const std::vector<Input>& inputs);
	void Execute(std::vector<Output>& outputs) noexcept;

private:
	std::vector<Input> mInputs;
};

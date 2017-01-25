#pragma once

#include <d3d12.h>

#include <DXUtils/D3DFactory.h>
#include <Utils/DebugUtils.h>

// Used to create Pipeline State Objects and Root Signatures (loaded from a shader file)
namespace PSOCreator {
	struct PSOParams {
		PSOParams() = default;
		~PSOParams() = default;
		PSOParams(const PSOParams&) = delete;
		const PSOParams& operator=(const PSOParams&) = delete;
		PSOParams(PSOParams&&) = delete;
		PSOParams& operator=(PSOParams&&) = delete;

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

	void CreatePSO(const PSOParams& psoParams, ID3D12PipelineState* &pso, ID3D12RootSignature* &rootSign) noexcept;
}

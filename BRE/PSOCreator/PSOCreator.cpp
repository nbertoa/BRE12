#include "PSOCreator.h"

#include <PSOManager/PSOManager.h>
#include <RootSignatureManager/RootSignatureManager.h>
#include <SettingsManager\SettingsManager.h>
#include <ShaderManager/ShaderManager.h>
#include <Utils/DebugUtils.h>

namespace {
	void BuildPSO(const PSOCreator::PSOParams& psoParams, ID3D12PipelineState* &pso, ID3D12RootSignature* &rootSign) noexcept {
		ASSERT(psoParams.ValidateData());

		ID3DBlob* rootSignBlob{ nullptr };
		ShaderManager::Get().LoadShaderFile(psoParams.mRootSignFilename, rootSignBlob);
		RootSignatureManager::Get().CreateRootSignature(*rootSignBlob, rootSign);

		D3D12_SHADER_BYTECODE vertexShader{};
		if (psoParams.mVSFilename != nullptr) {
			ShaderManager::Get().LoadShaderFile(psoParams.mVSFilename, vertexShader);
		}

		D3D12_SHADER_BYTECODE geomShader{};
		if (psoParams.mGSFilename != nullptr) {
			ShaderManager::Get().LoadShaderFile(psoParams.mGSFilename, geomShader);
		}

		D3D12_SHADER_BYTECODE domainShader{};
		if (psoParams.mDSFilename != nullptr) {
			ShaderManager::Get().LoadShaderFile(psoParams.mDSFilename, domainShader);
		}

		D3D12_SHADER_BYTECODE hullShader{};
		if (psoParams.mHSFilename != nullptr) {
			ShaderManager::Get().LoadShaderFile(psoParams.mHSFilename, hullShader);
		}

		D3D12_SHADER_BYTECODE pixelShader{};
		if (psoParams.mPSFilename != nullptr) {
			ShaderManager::Get().LoadShaderFile(psoParams.mPSFilename, pixelShader);
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.BlendState = psoParams.mBlendDesc;
		desc.DepthStencilState = psoParams.mDepthStencilDesc;
		desc.DS = domainShader;
		desc.DSVFormat = SettingsManager::sDepthStencilViewFormat;
		desc.GS = geomShader;
		desc.HS = hullShader;
		desc.InputLayout = { psoParams.mInputLayout.empty() 
			? nullptr 
			: psoParams.mInputLayout.data(), static_cast<std::uint32_t>(psoParams.mInputLayout.size()) };
		desc.NumRenderTargets = psoParams.mNumRenderTargets;
		desc.PrimitiveTopologyType = psoParams.mTopology;
		desc.pRootSignature = rootSign;
		desc.PS = pixelShader;
		desc.RasterizerState = psoParams.mRasterizerDesc;
		memcpy(desc.RTVFormats, psoParams.mRtFormats, sizeof(psoParams.mRtFormats));
		desc.SampleDesc = psoParams.mSampleDesc;
		desc.SampleMask = psoParams.mSampleMask;
		desc.VS = vertexShader;

		PSOManager::Get().CreateGraphicsPSO(desc, pso);

		ASSERT(pso != nullptr);
		ASSERT(rootSign != nullptr);
	}
}

namespace PSOCreator {
	void CreatePSO(const PSOParams& psoParams, ID3D12PipelineState* &pso, ID3D12RootSignature* &rootSign) noexcept {
		ASSERT(psoParams.ValidateData());
		BuildPSO(psoParams, pso, rootSign);
		ASSERT(pso != nullptr);
		ASSERT(rootSign != nullptr);
	}

	bool PSOParams::ValidateData() const noexcept {
		if (mNumRenderTargets == 0 || mNumRenderTargets > D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT || mRootSignFilename == nullptr) {
			return false;
		}

		for (std::uint32_t i = mNumRenderTargets; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
			if (mRtFormats[i] != DXGI_FORMAT_UNKNOWN) {
				return false;
			}
		}

		return true;
	}
}


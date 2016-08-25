#include "PSOCreator.h"

#include <GlobalData/Settings.h>
#include <MasterRender/MasterRender.h>
#include <PSOManager/PSOManager.h>
#include <RootSignatureManager/RootSignatureManager.h>
#include <ShaderManager/ShaderManager.h>
#include <Utils/DebugUtils.h>

namespace {
	struct PSOParams {
		PSOParams() = default;

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
		std::uint32_t mNumRenderTargets{ 0U };
		DXGI_FORMAT mRtFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{ DXGI_FORMAT_UNKNOWN };
		DXGI_SAMPLE_DESC mSampleDesc{ 1U, 0U };
		std::uint32_t mSampleMask{ UINT_MAX };
		D3D12_PRIMITIVE_TOPOLOGY_TYPE mTopology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
	};

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

	void BuildPSO(const PSOParams& psoParams, PSOCreator::PSOData& psoData) noexcept {
		ASSERT(psoParams.ValidateData());

		ID3DBlob* rootSignBlob{ nullptr };
		ShaderManager::Get().LoadShaderFile(psoParams.mRootSignFilename, rootSignBlob);
		RootSignatureManager::Get().CreateRootSignature(*rootSignBlob, psoData.mRootSign);

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
		desc.DSVFormat = MasterRender::DepthStencilFormat();
		desc.GS = geomShader;
		desc.HS = hullShader;
		desc.InputLayout = { psoParams.mInputLayout.empty() ? nullptr : psoParams.mInputLayout.data(), (std::uint32_t)psoParams.mInputLayout.size() };
		desc.NumRenderTargets = psoParams.mNumRenderTargets;
		desc.PrimitiveTopologyType = psoParams.mTopology;
		desc.pRootSignature = psoData.mRootSign;
		desc.PS = pixelShader;
		desc.RasterizerState = D3DFactory::DefaultRasterizerDesc();
		memcpy(desc.RTVFormats, psoParams.mRtFormats, sizeof(psoParams.mRtFormats));
		desc.SampleDesc = psoParams.mSampleDesc;
		desc.SampleMask = psoParams.mSampleMask;
		desc.VS = vertexShader;

		PSOManager::Get().CreateGraphicsPSO(desc, psoData.mPSO);

		ASSERT(psoData.ValidateData());
	}

	void CreatePSO(const PSOParams& psoParams, PSOCreator::PSOData& psoData) noexcept {
		ASSERT(psoParams.ValidateData());
		BuildPSO(psoParams, psoData);
		ASSERT(psoData.ValidateData());
	}
}

namespace PSOCreator {
	PSOData CommonPSOData::mPSOData[(std::uint32_t)Technique::NUM_TECHNIQUES];

	bool PSOData::ValidateData() const noexcept {
		return mPSO != nullptr && mRootSign != nullptr;
	}

	void Execute(const PSOParams& psoParams, PSOData& psoData) noexcept {
		ASSERT(psoParams.ValidateData());
		BuildPSO(psoParams, psoData);
		ASSERT(psoData.ValidateData());
	}

	void CommonPSOData::Init() noexcept {
		PSOParams psoParams{};
		psoParams.mInputLayout = D3DFactory::PosNormalTangentTexCoordInputLayout();
		psoParams.mPSFilename = "PSOCreator/Basic/PS.cso";
		psoParams.mRootSignFilename = "PSOCreator/Basic/RS.cso";
		psoParams.mVSFilename = "PSOCreator/Basic/VS.cso";
		psoParams.mNumRenderTargets = MasterRender::NumRenderTargets();
		memcpy(psoParams.mRtFormats, MasterRender::GeomPassBuffersFormats(), sizeof(DXGI_FORMAT) * psoParams.mNumRenderTargets);
		ASSERT(mPSOData[Technique::BASIC].mPSO == nullptr && mPSOData[Technique::BASIC].mRootSign == nullptr);
		PSOCreator::Execute(psoParams, mPSOData[Technique::BASIC]);

		psoParams = PSOParams{};
		psoParams.mInputLayout = D3DFactory::PosNormalTangentTexCoordInputLayout();
		psoParams.mPSFilename = "PSOCreator/TextureMapping/PS.cso";
		psoParams.mRootSignFilename = "PSOCreator/TextureMapping/RS.cso";
		psoParams.mVSFilename = "PSOCreator/TextureMapping/VS.cso";
		psoParams.mNumRenderTargets = MasterRender::NumRenderTargets();
		memcpy(psoParams.mRtFormats, MasterRender::GeomPassBuffersFormats(), sizeof(DXGI_FORMAT) * psoParams.mNumRenderTargets);
		ASSERT(mPSOData[Technique::TEXTURE_MAPPING].mPSO == nullptr && mPSOData[Technique::TEXTURE_MAPPING].mRootSign == nullptr);
		PSOCreator::Execute(psoParams, mPSOData[Technique::TEXTURE_MAPPING]);

		psoParams = PSOParams{};
		psoParams.mInputLayout = D3DFactory::PosNormalTangentTexCoordInputLayout();
		psoParams.mPSFilename = "PSOCreator/NormalMapping/PS.cso";
		psoParams.mRootSignFilename = "PSOCreator/NormalMapping/RS.cso";
		psoParams.mVSFilename = "PSOCreator/NormalMapping/VS.cso";
		psoParams.mNumRenderTargets = MasterRender::NumRenderTargets();
		memcpy(psoParams.mRtFormats, MasterRender::GeomPassBuffersFormats(), sizeof(DXGI_FORMAT) * psoParams.mNumRenderTargets);
		ASSERT(mPSOData[Technique::NORMAL_MAPPING].mPSO == nullptr && mPSOData[Technique::NORMAL_MAPPING].mRootSign == nullptr);
		PSOCreator::Execute(psoParams, mPSOData[Technique::NORMAL_MAPPING]);

		psoParams = PSOParams{};
		psoParams.mBlendDesc = D3DFactory::AlwaysBlendDesc();
		psoParams.mDepthStencilDesc = D3DFactory::DisableDepthStencilDesc();
		psoParams.mGSFilename = "PSOCreator/PunctualLight/GS.cso";
		psoParams.mPSFilename = "PSOCreator/PunctualLight/PS.cso";
		psoParams.mRootSignFilename = "PSOCreator/PunctualLight/RS.cso";
		psoParams.mVSFilename = "PSOCreator/PunctualLight/VS.cso";
		psoParams.mNumRenderTargets = 1U;
		psoParams.mRtFormats[0U] = MasterRender::BackBufferRTFormat();
		const std::size_t rtCount{ _countof(psoParams.mRtFormats) };
		for (std::size_t i = 1UL; i < rtCount; ++i) {
			psoParams.mRtFormats[i] = DXGI_FORMAT_UNKNOWN;
		}
		psoParams.mTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		ASSERT(mPSOData[Technique::PUNCTUAL_LIGHT].mPSO == nullptr && mPSOData[Technique::PUNCTUAL_LIGHT].mRootSign == nullptr);
		CreatePSO(psoParams, mPSOData[Technique::PUNCTUAL_LIGHT]);
	}

	const PSOData& CommonPSOData::GetData(const CommonPSOData::Technique tech) noexcept {
		ASSERT(tech < CommonPSOData::Technique::NUM_TECHNIQUES);
		return mPSOData[tech];
	}

}


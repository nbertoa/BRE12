#include "PSOCreator.h"

#include <App/MasterRender.h>
#include <GlobalData/Settings.h>
#include <PSOManager/PSOManager.h>
#include <RootSignatureManager/RootSignatureManager.h>
#include <ShaderManager/ShaderManager.h>
#include <Utils/DebugUtils.h>

namespace {
	void BuildPSO(const PSOCreator::Input& input, PSOCreator::Output& output) noexcept {
		ASSERT(input.ValidateData());

		ID3DBlob* rootSignBlob{ nullptr };
		ShaderManager::Get().LoadShaderFile(input.mRootSignFilename, rootSignBlob);
		RootSignatureManager::Get().CreateRootSignature(*rootSignBlob, output.mRootSign);

		D3D12_SHADER_BYTECODE vertexShader{};
		if (input.mVSFilename != nullptr) {
			ShaderManager::Get().LoadShaderFile(input.mVSFilename, vertexShader);
		}

		D3D12_SHADER_BYTECODE geomShader{};
		if (input.mGSFilename != nullptr) {
			ShaderManager::Get().LoadShaderFile(input.mGSFilename, geomShader);
		}

		D3D12_SHADER_BYTECODE domainShader{};
		if (input.mDSFilename != nullptr) {
			ShaderManager::Get().LoadShaderFile(input.mDSFilename, domainShader);
		}

		D3D12_SHADER_BYTECODE hullShader{};
		if (input.mHSFilename != nullptr) {
			ShaderManager::Get().LoadShaderFile(input.mHSFilename, hullShader);
		}

		D3D12_SHADER_BYTECODE pixelShader{};
		if (input.mPSFilename != nullptr) {
			ShaderManager::Get().LoadShaderFile(input.mPSFilename, pixelShader);
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.BlendState = input.mBlendDesc;
		desc.DepthStencilState = input.mDepthStencilDesc;
		desc.DS = domainShader;
		desc.DSVFormat = MasterRender::DepthStencilFormat();
		desc.GS = geomShader;
		desc.HS = hullShader;
		desc.InputLayout = { input.mInputLayout.empty() ? nullptr : input.mInputLayout.data(), (std::uint32_t)input.mInputLayout.size() };		
		desc.NumRenderTargets = input.mNumRenderTargets;
		desc.PrimitiveTopologyType = input.mTopology;
		desc.pRootSignature = output.mRootSign;
		desc.PS = pixelShader;
		desc.RasterizerState = D3DFactory::DefaultRasterizerDesc();
		memcpy(desc.RTVFormats, input.mRtFormats, sizeof(input.mRtFormats));
		desc.SampleDesc = input.mSampleDesc;
		desc.SampleMask = input.mSampleMask;
		desc.VS = vertexShader;

		PSOManager::Get().CreateGraphicsPSO(desc, output.mPSO);
	}
}

namespace PSOCreator {
	bool Input::ValidateData() const noexcept {
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

	void Execute(const Input& input, Output& output) noexcept {
		ASSERT(input.ValidateData());
		BuildPSO(input, output);
	}

	Output CommonPSOData::mPSOData[(std::uint32_t)Technique::NUM_TECHNIQUES];

	void CommonPSOData::Init() noexcept {
		Input psoCreatorInput;
		psoCreatorInput.mInputLayout = D3DFactory::PosNormalTangentTexCoordInputLayout();
		psoCreatorInput.mPSFilename = "PSOCreator/Black/PS.cso";
		psoCreatorInput.mRootSignFilename = "PSOCreator/Black/RS.cso";
		psoCreatorInput.mVSFilename = "PSOCreator/Black/VS.cso";
		psoCreatorInput.mNumRenderTargets = MasterRender::NumRenderTargets();
		memcpy(psoCreatorInput.mRtFormats, MasterRender::GeomPassBuffersFormats(), sizeof(DXGI_FORMAT) * psoCreatorInput.mNumRenderTargets);
		ASSERT(mPSOData[Technique::BLACK].mPSO == nullptr && mPSOData[Technique::BLACK].mRootSign == nullptr);
		PSOCreator::Execute(psoCreatorInput, mPSOData[Technique::BLACK]);

		psoCreatorInput = Input{};
		psoCreatorInput.mInputLayout = D3DFactory::PosNormalTangentTexCoordInputLayout();
		psoCreatorInput.mPSFilename = "PSOCreator/Basic/PS.cso";
		psoCreatorInput.mRootSignFilename = "PSOCreator/Basic/RS.cso";
		psoCreatorInput.mVSFilename = "PSOCreator/Basic/VS.cso";
		psoCreatorInput.mNumRenderTargets = MasterRender::NumRenderTargets();
		memcpy(psoCreatorInput.mRtFormats, MasterRender::GeomPassBuffersFormats(), sizeof(DXGI_FORMAT) * psoCreatorInput.mNumRenderTargets);
		ASSERT(mPSOData[Technique::BASIC].mPSO == nullptr && mPSOData[Technique::BASIC].mRootSign == nullptr);
		PSOCreator::Execute(psoCreatorInput, mPSOData[Technique::BASIC]);

		psoCreatorInput = Input{};
		psoCreatorInput.mGSFilename = "PSOCreator/PunctualLight/GS.cso";
		psoCreatorInput.mPSFilename = "PSOCreator/PunctualLight/PS.cso";
		psoCreatorInput.mRootSignFilename = "PSOCreator/PunctualLight/RS.cso";
		psoCreatorInput.mVSFilename = "PSOCreator/PunctualLight/VS.cso";
		psoCreatorInput.mNumRenderTargets = 1U;
		psoCreatorInput.mRtFormats[0U] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; /*TODO*/ //MasterRender::BackBufferFormat();
		const std::size_t rtCount{ _countof(psoCreatorInput.mRtFormats) };
		for (std::size_t i = 1UL; i < rtCount; ++i) {
			psoCreatorInput.mRtFormats[i] = DXGI_FORMAT_UNKNOWN;
		}
		psoCreatorInput.mTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		ASSERT(mPSOData[Technique::PUNCTUAL_LIGHT].mPSO == nullptr && mPSOData[Technique::PUNCTUAL_LIGHT].mRootSign == nullptr);
		PSOCreator::Execute(psoCreatorInput, mPSOData[Technique::PUNCTUAL_LIGHT]);
	}

	const Output& CommonPSOData::GetData(const CommonPSOData::Technique tech) noexcept {
		ASSERT(tech < CommonPSOData::Technique::NUM_TECHNIQUES);
		return mPSOData[tech];
	}

}


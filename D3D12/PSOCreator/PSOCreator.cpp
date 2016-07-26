#include "PSOCreator.h"

#include <GlobalData/Settings.h>
#include <PSOManager/PSOManager.h>
#include <RootSignatureManager/RootSignatureManager.h>
#include <ShaderManager/ShaderManager.h>
#include <Utils/DebugUtils.h>

namespace {
	void BuildPSO(const PSOCreator::Input& input, PSOCreator::Output& output) noexcept {
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
		desc.DSVFormat = Settings::sDepthStencilFormat;
		desc.GS = geomShader;
		desc.HS = hullShader;
		desc.InputLayout = { input.mInputLayout.data(), (std::uint32_t)input.mInputLayout.size() };
		desc.NumRenderTargets = input.mNumRenderTargets;
		desc.PrimitiveTopologyType = input.mTopology;
		desc.pRootSignature = output.mRootSign;
		desc.PS = pixelShader;
		desc.RasterizerState = D3DFactory::DefaultRasterizerDesc();
		memcpy(desc.RTVFormats, Settings::sRTVFormats, sizeof(Settings::sRTVFormats));
		desc.SampleDesc = input.mSampleDesc;
		desc.SampleMask = input.mSampleMask;
		desc.VS = vertexShader;

		PSOManager::Get().CreateGraphicsPSO(desc, output.mPSO);
	}
}

namespace PSOCreator {
	bool Input::ValidateData() const noexcept {
		return mInputLayout.empty() == false && mRootSignFilename != nullptr;
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
		ASSERT(mPSOData[Technique::BLACK].mPSO == nullptr && mPSOData[Technique::BLACK].mRootSign == nullptr);
		PSOCreator::Execute(psoCreatorInput, mPSOData[Technique::BLACK]);

		psoCreatorInput = Input{};
		psoCreatorInput.mInputLayout = D3DFactory::PosNormalTangentTexCoordInputLayout();
		psoCreatorInput.mPSFilename = "PSOCreator/Basic/PS.cso";
		psoCreatorInput.mRootSignFilename = "PSOCreator/Basic/RS.cso";
		psoCreatorInput.mVSFilename = "PSOCreator/Basic/VS.cso";
		ASSERT(mPSOData[Technique::BASIC].mPSO == nullptr && mPSOData[Technique::BASIC].mRootSign == nullptr);
		PSOCreator::Execute(psoCreatorInput, mPSOData[Technique::BASIC]);
	}

	const Output& CommonPSOData::GetData(const CommonPSOData::Technique tech) noexcept {
		ASSERT(tech < CommonPSOData::Technique::NUM_TECHNIQUES);
		return mPSOData[tech];
	}

}


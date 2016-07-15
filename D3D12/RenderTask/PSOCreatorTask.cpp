#include "PSOCreatorTask.h"

#include <GlobalData/Settings.h>
#include <PSOManager/PSOManager.h>
#include <RootSignatureManager/RootSignatureManager.h>
#include <ShaderManager/ShaderManager.h>
#include <Utils/DebugUtils.h>

namespace {
	void BuildPSO(const PSOCreatorTask::Input& input, PSOCreatorTask::Output& output) noexcept {
		ID3DBlob* rootSignBlob{ nullptr };
		ShaderManager::gManager->LoadShaderFile(input.mRootSignFilename, rootSignBlob);
		RootSignatureManager::gManager->CreateRootSignature(*rootSignBlob, output.mRootSign);

		D3D12_SHADER_BYTECODE vertexShader{};
		if (input.mVSFilename != nullptr) {
			ShaderManager::gManager->LoadShaderFile(input.mVSFilename, vertexShader);
		}

		D3D12_SHADER_BYTECODE geomShader{};
		if (input.mGSFilename != nullptr) {
			ShaderManager::gManager->LoadShaderFile(input.mGSFilename, geomShader);
		}

		D3D12_SHADER_BYTECODE domainShader{};
		if (input.mDSFilename != nullptr) {
			ShaderManager::gManager->LoadShaderFile(input.mDSFilename, domainShader);
		}

		D3D12_SHADER_BYTECODE hullShader{};
		if (input.mHSFilename != nullptr) {
			ShaderManager::gManager->LoadShaderFile(input.mHSFilename, hullShader);
		}

		D3D12_SHADER_BYTECODE pixelShader{};
		if (input.mPSFilename != nullptr) {
			ShaderManager::gManager->LoadShaderFile(input.mPSFilename, pixelShader);
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

		PSOManager::gManager->CreateGraphicsPSO(desc, output.mPSO);
	}
}

bool PSOCreatorTask::Input::ValidateData() const noexcept {
	return mInputLayout.empty() == false && mRootSignFilename != nullptr;
}

PSOCreatorTask::PSOCreatorTask(const std::vector<Input>& inputs) 
	: mInputs(inputs)
{
}

void PSOCreatorTask::Execute(std::vector<Output>& outputs) noexcept {
	ASSERT(mInputs.empty() == false);

	const std::size_t count{ mInputs.size() };
	outputs.resize(count);
	for (std::size_t i = 0UL; i < count; ++i) {
		BuildPSO(mInputs[i], outputs[i]);
	}
}
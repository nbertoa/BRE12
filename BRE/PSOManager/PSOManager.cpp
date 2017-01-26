#include "PSOManager.h"

#include <memory>

#include <DirectXManager/DirectXManager.h>
#include <RootSignatureManager/RootSignatureManager.h>
#include <SettingsManager\SettingsManager.h>
#include <ShaderManager/ShaderManager.h>
#include <Utils/DebugUtils.h>
#include <Utils/NumberGeneration.h>

namespace {
	std::unique_ptr<PSOManager> gManager{ nullptr };
}

PSOManager& PSOManager::Create() noexcept {
	ASSERT(gManager == nullptr);
	gManager.reset(new PSOManager());
	return *gManager.get();
}

PSOManager& PSOManager::Get() noexcept {
	ASSERT(gManager != nullptr);
	return *gManager.get();
}

bool PSOManager::PSOCreationData::IsDataValid() const noexcept {
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

std::size_t PSOManager::CreateGraphicsPSO(
	const PSOManager::PSOCreationData& psoData,
	ID3D12PipelineState* &pso,
	ID3D12RootSignature* &rootSign) noexcept
{
	ASSERT(psoData.IsDataValid());

	ID3DBlob* rootSignatureBlob{ nullptr };
	ShaderManager::Get().LoadShaderFile(psoData.mRootSignFilename, rootSignatureBlob);
	RootSignatureManager::Get().CreateRootSignatureFromBlob(*rootSignatureBlob, rootSign);

	D3D12_SHADER_BYTECODE vertexShader{};
	if (psoData.mVSFilename != nullptr) {
		ShaderManager::Get().LoadShaderFile(psoData.mVSFilename, vertexShader);
	}

	D3D12_SHADER_BYTECODE geomShader{};
	if (psoData.mGSFilename != nullptr) {
		ShaderManager::Get().LoadShaderFile(psoData.mGSFilename, geomShader);
	}

	D3D12_SHADER_BYTECODE domainShader{};
	if (psoData.mDSFilename != nullptr) {
		ShaderManager::Get().LoadShaderFile(psoData.mDSFilename, domainShader);
	}

	D3D12_SHADER_BYTECODE hullShader{};
	if (psoData.mHSFilename != nullptr) {
		ShaderManager::Get().LoadShaderFile(psoData.mHSFilename, hullShader);
	}

	D3D12_SHADER_BYTECODE pixelShader{};
	if (psoData.mPSFilename != nullptr) {
		ShaderManager::Get().LoadShaderFile(psoData.mPSFilename, pixelShader);
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescriptor = {};
	psoDescriptor.BlendState = psoData.mBlendDesc;
	psoDescriptor.DepthStencilState = psoData.mDepthStencilDesc;
	psoDescriptor.DS = domainShader;
	psoDescriptor.DSVFormat = SettingsManager::sDepthStencilViewFormat;
	psoDescriptor.GS = geomShader;
	psoDescriptor.HS = hullShader;
	psoDescriptor.InputLayout =
	{
		psoData.mInputLayout.empty()
		? nullptr
		: psoData.mInputLayout.data(), static_cast<std::uint32_t>(psoData.mInputLayout.size())
	};
	psoDescriptor.NumRenderTargets = psoData.mNumRenderTargets;
	psoDescriptor.PrimitiveTopologyType = psoData.mTopology;
	psoDescriptor.pRootSignature = rootSign;
	psoDescriptor.PS = pixelShader;
	psoDescriptor.RasterizerState = psoData.mRasterizerDesc;
	memcpy(psoDescriptor.RTVFormats, psoData.mRtFormats, sizeof(psoData.mRtFormats));
	psoDescriptor.SampleDesc = psoData.mSampleDesc;
	psoDescriptor.SampleMask = psoData.mSampleMask;
	psoDescriptor.VS = vertexShader;

	const std::size_t id = CreateGraphicsPSOByDescriptor(psoDescriptor, pso);

	ASSERT(pso != nullptr);
	ASSERT(rootSign != nullptr);

	return id;
}

std::size_t PSOManager::CreateGraphicsPSOByDescriptor(
	const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDescriptor, 
	ID3D12PipelineState* &pso) noexcept 
{
	mMutex.lock();
	CHECK_HR(DirectXManager::GetDevice().CreateGraphicsPipelineState(&psoDescriptor, IID_PPV_ARGS(&pso)));
	mMutex.unlock();

	const std::size_t id{ NumberGeneration::GetIncrementalSizeT() };
	PSOById::accessor accessor;
#ifdef _DEBUG
	mPSOById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mPSOById.insert(accessor, id);
	accessor->second = Microsoft::WRL::ComPtr<ID3D12PipelineState>(pso);
	accessor.release();

	return id;
}

ID3D12PipelineState& PSOManager::GetGraphicsPSO(const std::size_t id) noexcept {
	PSOById::accessor accessor;
	mPSOById.find(accessor, id);
	ASSERT(!accessor.empty());
	ID3D12PipelineState* state{ accessor->second.Get() };
	accessor.release();

	return *state;
}
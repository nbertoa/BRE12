#include "PSOManager.h"

#include <DirectXManager/DirectXManager.h>
#include <SettingsManager\SettingsManager.h>
#include <Utils/DebugUtils.h>

PSOManager::PSOs PSOManager::mPSOs;
std::mutex PSOManager::mMutex;

PSOManager::~PSOManager() {
	for (ID3D12PipelineState* pso : mPSOs) {
		ASSERT(pso != nullptr);
		pso->Release();
		delete pso;
	}
}

bool PSOManager::PSOCreationData::IsDataValid() const noexcept {
	if (mNumRenderTargets == 0 || 
		mNumRenderTargets > D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT || 
		mRootSignature == nullptr) {
		return false;
	}

	for (std::uint32_t i = mNumRenderTargets; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
		if (mRenderTargetFormats[i] != DXGI_FORMAT_UNKNOWN) {
			return false;
		}
	}

	return true;
}

ID3D12PipelineState& PSOManager::CreateGraphicsPSO(const PSOManager::PSOCreationData& psoData) noexcept {
	ASSERT(psoData.IsDataValid());
		
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescriptor = {};
	psoDescriptor.BlendState = psoData.mBlendDescriptor;
	psoDescriptor.DepthStencilState = psoData.mDepthStencilDescriptor;
	psoDescriptor.DS = psoData.mDomainShaderBytecode;
	psoDescriptor.DSVFormat = SettingsManager::sDepthStencilViewFormat;
	psoDescriptor.GS = psoData.mGeometryShaderBytecode;
	psoDescriptor.HS = psoData.mHullShaderBytecode;
	psoDescriptor.InputLayout =
	{
		psoData.mInputLayoutDescriptors.empty()
		? nullptr
		: psoData.mInputLayoutDescriptors.data(), static_cast<std::uint32_t>(psoData.mInputLayoutDescriptors.size())
	};
	psoDescriptor.NumRenderTargets = psoData.mNumRenderTargets;
	psoDescriptor.PrimitiveTopologyType = psoData.mPrimitiveTopologyType;
	psoDescriptor.pRootSignature = psoData.mRootSignature;
	psoDescriptor.PS = psoData.mPixelShaderBytecode;
	psoDescriptor.RasterizerState = psoData.mRasterizerDescriptor;
	memcpy(psoDescriptor.RTVFormats, psoData.mRenderTargetFormats, sizeof(psoData.mRenderTargetFormats));
	psoDescriptor.SampleDesc = psoData.mSampleDescriptor;
	psoDescriptor.SampleMask = psoData.mSampleMask;
	psoDescriptor.VS = psoData.mVertexShaderBytecode;

	return CreateGraphicsPSOByDescriptor(psoDescriptor);
}

ID3D12PipelineState& PSOManager::CreateGraphicsPSOByDescriptor(
	const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDescriptor) noexcept 
{
	ID3D12PipelineState* pso{ nullptr };

	mMutex.lock();
	CHECK_HR(DirectXManager::GetDevice().CreateGraphicsPipelineState(&psoDescriptor, IID_PPV_ARGS(&pso)));
	mMutex.unlock();

	ASSERT(pso != nullptr);
	mPSOs.insert(pso);

	return *pso;
}
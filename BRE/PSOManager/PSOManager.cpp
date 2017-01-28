#include "PSOManager.h"

#include <memory>

#include <DirectXManager/DirectXManager.h>
#include <SettingsManager\SettingsManager.h>
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

std::size_t PSOManager::CreateGraphicsPSO(
	const PSOManager::PSOCreationData& psoData,
	ID3D12PipelineState* &pso) noexcept
{
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

	const std::size_t id = CreateGraphicsPSOByDescriptor(psoDescriptor, pso);

	ASSERT(pso != nullptr);

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
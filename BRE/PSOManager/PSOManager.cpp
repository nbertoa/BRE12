#include "PSOManager.h"

#include <DirectXManager/DirectXManager.h>
#include <ApplicationSettings\ApplicationSettings.h>
#include <Utils/DebugUtils.h>

namespace BRE {
tbb::concurrent_unordered_set<ID3D12PipelineState*> PSOManager::mPSOs;
std::mutex PSOManager::mMutex;

void
PSOManager::Clear() noexcept
{
    for (ID3D12PipelineState* pso : mPSOs) {
        BRE_ASSERT(pso != nullptr);
        pso->Release();
    }

    mPSOs.clear();
}

bool
PSOManager::PSOCreationData::IsDataValid() const noexcept
{
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

ID3D12PipelineState&
PSOManager::CreateGraphicsPSO(const PSOManager::PSOCreationData& psoData) noexcept
{
    BRE_ASSERT(psoData.IsDataValid());

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescriptor = {};
    psoDescriptor.BlendState = psoData.mBlendDescriptor;
    psoDescriptor.DepthStencilState = psoData.mDepthStencilDescriptor;
    psoDescriptor.DS = psoData.mDomainShaderBytecode;
    psoDescriptor.DSVFormat = ApplicationSettings::sDepthStencilViewFormat;
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

ID3D12PipelineState&
PSOManager::CreateGraphicsPSOByDescriptor(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDescriptor) noexcept
{
    ID3D12PipelineState* pso{ nullptr };

    mMutex.lock();
    BRE_CHECK_HR(DirectXManager::GetDevice().CreateGraphicsPipelineState(&psoDescriptor, IID_PPV_ARGS(&pso)));
    mMutex.unlock();

    BRE_ASSERT(pso != nullptr);
    mPSOs.insert(pso);

    return *pso;
}
}
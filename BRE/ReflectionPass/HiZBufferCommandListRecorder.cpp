#include "HiZBufferCommandListRecorder.h"

#include <d3d12.h>

#include <CommandListExecutor\CommandListExecutor.h>
#include <DescriptorManager\CbvSrvUavDescriptorManager.h>
#include <DirectXManager\DirectXManager.h>
#include <PSOManager/PSOManager.h>
#include <ResourceManager/UploadBufferManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <ShaderManager\ShaderManager.h>
#include <Utils/DebugUtils.h>

using namespace DirectX;

namespace BRE {
// Root Signature:
// "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL), " \ 0 -> Upper level hi-z resource

namespace {
ID3D12PipelineState* sPSO{ nullptr };
ID3D12RootSignature* sRootSignature{ nullptr };
}

void
HiZBufferCommandListRecorder::InitSharedPSOAndRootSignature() noexcept
{
    BRE_ASSERT(sPSO == nullptr);
    BRE_ASSERT(sRootSignature == nullptr);

    PSOManager::PSOCreationData psoData{};
    psoData.mDepthStencilDescriptor = D3DFactory::GetDisabledDepthStencilDesc();

    psoData.mPixelShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("ReflectionPass/Shaders/PS.cso");
    psoData.mVertexShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("ReflectionPass/Shaders/VS.cso");

    ID3DBlob* rootSignatureBlob = &ShaderManager::LoadShaderFileAndGetBlob("ReflectionPass/Shaders/RS.cso");
    psoData.mRootSignature = &RootSignatureManager::CreateRootSignatureFromBlob(*rootSignatureBlob);
    sRootSignature = psoData.mRootSignature;

    psoData.mNumRenderTargets = 1U;
    psoData.mRenderTargetFormats[0U] = ApplicationSettings::sDepthStencilSRVFormat;
    for (std::size_t i = psoData.mNumRenderTargets; i < _countof(psoData.mRenderTargetFormats); ++i) {
        psoData.mRenderTargetFormats[i] = DXGI_FORMAT_UNKNOWN;
    }
    psoData.mPrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    sPSO = &PSOManager::CreateGraphicsPSO(psoData);

    BRE_ASSERT(sPSO != nullptr);
    BRE_ASSERT(sRootSignature != nullptr);
}

void
HiZBufferCommandListRecorder::Init(const D3D12_GPU_DESCRIPTOR_HANDLE& upperLevelBufferShaderResourceView,
                                   const D3D12_CPU_DESCRIPTOR_HANDLE& lowerLevelBufferRenderTargetView) noexcept
{
    BRE_ASSERT(IsDataValid() == false);

    mUpperLevelBufferShaderResourceView = upperLevelBufferShaderResourceView;
    mLowerLevelBufferRenderTargetView = lowerLevelBufferRenderTargetView;

    BRE_ASSERT(IsDataValid());
}

std::uint32_t
HiZBufferCommandListRecorder::RecordAndPushCommandLists() noexcept
{
    BRE_ASSERT(IsDataValid());
    BRE_ASSERT(sPSO != nullptr);
    BRE_ASSERT(sRootSignature != nullptr);

    ID3D12GraphicsCommandList& commandList = mCommandListPerFrame.ResetCommandListWithNextCommandAllocator(sPSO);

    commandList.RSSetViewports(1U, &ApplicationSettings::sScreenViewport);
    commandList.RSSetScissorRects(1U, &ApplicationSettings::sScissorRect);
    commandList.OMSetRenderTargets(1U, &mLowerLevelBufferRenderTargetView, false, nullptr);

    ID3D12DescriptorHeap* heaps[] = { &CbvSrvUavDescriptorManager::GetDescriptorHeap() };
    commandList.SetDescriptorHeaps(_countof(heaps), heaps);

    commandList.SetGraphicsRootSignature(sRootSignature);
    commandList.SetGraphicsRootDescriptorTable(0U, mUpperLevelBufferShaderResourceView);

    commandList.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList.DrawInstanced(6U, 1U, 0U, 0U);

    commandList.Close();
    CommandListExecutor::Get().PushCommandList(commandList);

    return 1U;
}

bool
HiZBufferCommandListRecorder::IsDataValid() const noexcept
{
    const bool result =
        mUpperLevelBufferShaderResourceView.ptr != 0UL &&
        mLowerLevelBufferRenderTargetView.ptr != 0UL;

    return result;
}
}
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
// "DescriptorTable(CBV(b0), visibility = SHADER_VISIBILITY_VERTEX), " \ 0 -> Object CBuffers
// "CBV(b1, visibility = SHADER_VISIBILITY_VERTEX), " \ 1 > Frame CBuffer
// "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL), " \ 2 -> Cube Map texture

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

    // The camera is inside the sky sphere, so just turn off culling.
    psoData.mRasterizerDescriptor.CullMode = D3D12_CULL_MODE_NONE;

    // Make sure the depth function is LESS_EQUAL and not just GREATER.  
    // Otherwise, the normalized depth values at z = 1 (NDC) will 
    // fail the depth test if the depth buffer was cleared to 1.
    psoData.mDepthStencilDescriptor.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoData.mInputLayoutDescriptors = D3DFactory::GetPositionNormalTangentTexCoordInputLayout();

    psoData.mPixelShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("SkyBoxPass/Shaders/PS.cso");
    psoData.mVertexShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("SkyBoxPass/Shaders/VS.cso");

    ID3DBlob* rootSignatureBlob = &ShaderManager::LoadShaderFileAndGetBlob("SkyBoxPass/Shaders/RS.cso");
    psoData.mRootSignature = &RootSignatureManager::CreateRootSignatureFromBlob(*rootSignatureBlob);
    sRootSignature = psoData.mRootSignature;

    psoData.mNumRenderTargets = 1U;
    psoData.mRenderTargetFormats[0U] = ApplicationSettings::sColorBufferFormat;
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
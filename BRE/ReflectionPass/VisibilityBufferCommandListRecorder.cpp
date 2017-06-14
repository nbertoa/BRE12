#include "VisibilityBufferCommandListRecorder.h"

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
// "DescriptorTable(SRV(t1), visibility = SHADER_VISIBILITY_PIXEL), " \ 1 -> Upper level visibility resource

namespace {
ID3D12PipelineState* sPSO{ nullptr };
ID3D12RootSignature* sRootSignature{ nullptr };
}

void
VisibilityBufferCommandListRecorder::InitSharedPSOAndRootSignature() noexcept
{
    BRE_ASSERT(sPSO == nullptr);
    BRE_ASSERT(sRootSignature == nullptr);

    PSOManager::PSOCreationData psoData{};
    psoData.mDepthStencilDescriptor = D3DFactory::GetDisabledDepthStencilDesc();

    psoData.mPixelShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("ReflectionPass/Shaders/VisibilityBuffer/PS.cso");
    psoData.mVertexShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("ReflectionPass/Shaders/VisibilityBuffer/VS.cso");

    ID3DBlob* rootSignatureBlob = &ShaderManager::LoadShaderFileAndGetBlob("ReflectionPass/Shaders/VisibilityBuffer/RS.cso");
    psoData.mRootSignature = &RootSignatureManager::CreateRootSignatureFromBlob(*rootSignatureBlob);
    sRootSignature = psoData.mRootSignature;

    psoData.mNumRenderTargets = 1U;
    psoData.mRenderTargetFormats[0U] = DXGI_FORMAT_R8_UNORM;
    for (std::size_t i = psoData.mNumRenderTargets; i < _countof(psoData.mRenderTargetFormats); ++i) {
        psoData.mRenderTargetFormats[i] = DXGI_FORMAT_UNKNOWN;
    }
    psoData.mPrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    sPSO = &PSOManager::CreateGraphicsPSO(psoData);

    BRE_ASSERT(sPSO != nullptr);
    BRE_ASSERT(sRootSignature != nullptr);
}

void
VisibilityBufferCommandListRecorder::Init(const D3D12_GPU_DESCRIPTOR_HANDLE& upperLevelHiZBufferShaderResourceView,
                                          const D3D12_GPU_DESCRIPTOR_HANDLE& upperLevelVisibilityBufferShaderResourceView,
                                          const D3D12_CPU_DESCRIPTOR_HANDLE& lowerLevelVisibilityBufferRenderTargetView) noexcept
{
    BRE_ASSERT(IsDataValid() == false);

    mUpperLevelHiZBufferShaderResourceView = upperLevelHiZBufferShaderResourceView;
    mUpperLevelVisibilityBufferShaderResourceView = upperLevelVisibilityBufferShaderResourceView;
    mLowerLevelVisibilityBufferRenderTargetView = lowerLevelVisibilityBufferRenderTargetView;

    BRE_ASSERT(IsDataValid());
}

std::uint32_t
VisibilityBufferCommandListRecorder::RecordAndPushCommandLists() noexcept
{
    BRE_ASSERT(IsDataValid());
    BRE_ASSERT(sPSO != nullptr);
    BRE_ASSERT(sRootSignature != nullptr);

    ID3D12GraphicsCommandList& commandList = mCommandListPerFrame.ResetCommandListWithNextCommandAllocator(sPSO);

    commandList.RSSetViewports(1U, &ApplicationSettings::sScreenViewport);
    commandList.RSSetScissorRects(1U, &ApplicationSettings::sScissorRect);
    commandList.OMSetRenderTargets(1U, &mLowerLevelVisibilityBufferRenderTargetView, false, nullptr);

    ID3D12DescriptorHeap* heaps[] = { &CbvSrvUavDescriptorManager::GetDescriptorHeap() };
    commandList.SetDescriptorHeaps(_countof(heaps), heaps);

    commandList.SetGraphicsRootSignature(sRootSignature);
    commandList.SetGraphicsRootDescriptorTable(0U, mUpperLevelHiZBufferShaderResourceView);
    commandList.SetGraphicsRootDescriptorTable(1U, mUpperLevelHiZBufferShaderResourceView);

    commandList.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList.DrawInstanced(6U, 1U, 0U, 0U);

    commandList.Close();
    CommandListExecutor::Get().PushCommandList(commandList);

    return 1U;
}

bool
VisibilityBufferCommandListRecorder::IsDataValid() const noexcept
{
    const bool result =
        mUpperLevelHiZBufferShaderResourceView.ptr != 0UL &&
        mUpperLevelVisibilityBufferShaderResourceView.ptr != 0UL &&
        mLowerLevelVisibilityBufferRenderTargetView.ptr != 0UL;

    return result;
}
}
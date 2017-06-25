#include "BlurCommandListRecorder.h"

#include <d3d12.h>
#include <DirectXMath.h>

#include <AmbientOcclusionPass\AmbientOcclusionSettings.h>
#include <AmbientOcclusionPass\Shaders\BlurCBuffer.h>
#include <ApplicationSettings\ApplicationSettings.h>
#include <CommandListExecutor\CommandListExecutor.h>
#include <DescriptorManager\CbvSrvUavDescriptorManager.h>
#include <PSOManager/PSOManager.h>
#include <ResourceManager/UploadBufferManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <ShaderManager\ShaderManager.h>
#include <Utils/DebugUtils.h>

namespace BRE {
// Root Signature:
// "CBV(b0, visibility = SHADER_VISIBILITY_PIXEL), " \ 0 -> Blur CBuffer
// "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL)" 1 -> Color Buffer Texture

namespace {
ID3D12PipelineState* sPSO{ nullptr };
ID3D12RootSignature* sRootSignature{ nullptr };
}

void
BlurCommandListRecorder::InitSharedPSOAndRootSignature() noexcept
{
    BRE_ASSERT(sPSO == nullptr);
    BRE_ASSERT(sRootSignature == nullptr);

    PSOManager::PSOCreationData psoData{};
    psoData.mDepthStencilDescriptor = D3DFactory::GetDisabledDepthStencilDesc();

    psoData.mPixelShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("AmbientOcclusionPass/Shaders/Blur/PS.cso");
    psoData.mVertexShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("AmbientOcclusionPass/Shaders/Blur/VS.cso");

    ID3DBlob* rootSignatureBlob = &ShaderManager::LoadShaderFileAndGetBlob("AmbientOcclusionPass/Shaders/Blur/RS.cso");
    psoData.mRootSignature = &RootSignatureManager::CreateRootSignatureFromBlob(*rootSignatureBlob);
    sRootSignature = psoData.mRootSignature;

    psoData.mNumRenderTargets = 1U;
    psoData.mRenderTargetFormats[0U] = DXGI_FORMAT_R16_UNORM;
    for (std::size_t i = psoData.mNumRenderTargets; i < _countof(psoData.mRenderTargetFormats); ++i) {
        psoData.mRenderTargetFormats[i] = DXGI_FORMAT_UNKNOWN;
    }
    psoData.mPrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    sPSO = &PSOManager::CreateGraphicsPSO(psoData);

    BRE_ASSERT(sPSO != nullptr);
    BRE_ASSERT(sRootSignature != nullptr);
}

void
BlurCommandListRecorder::Init(const D3D12_GPU_DESCRIPTOR_HANDLE& ambientAccessibilityBufferShaderResourceView,
                              const D3D12_CPU_DESCRIPTOR_HANDLE& outputAmbientAccessibilityBufferRenderTargetView) noexcept
{
    BRE_ASSERT(IsDataValid() == false);

    mAmbientAccessibilityBufferShaderResourceView = ambientAccessibilityBufferShaderResourceView;
    mOutputAmbientAccessibilityBufferRenderTargetView = outputAmbientAccessibilityBufferRenderTargetView;

    InitBlurCBuffer();

    BRE_ASSERT(IsDataValid());
}

std::uint32_t
BlurCommandListRecorder::RecordAndPushCommandLists() noexcept
{
    BRE_ASSERT(IsDataValid());
    BRE_ASSERT(sPSO != nullptr);
    BRE_ASSERT(sRootSignature != nullptr);

    ID3D12GraphicsCommandList& commandList = mCommandListPerFrame.ResetCommandListWithNextCommandAllocator(sPSO);

    commandList.RSSetViewports(1U, &ApplicationSettings::sScreenViewport);
    commandList.RSSetScissorRects(1U, &ApplicationSettings::sScissorRect);
    commandList.OMSetRenderTargets(1U,
                                   &mOutputAmbientAccessibilityBufferRenderTargetView,
                                   false,
                                   nullptr);

    ID3D12DescriptorHeap* heaps[] = { &CbvSrvUavDescriptorManager::GetDescriptorHeap() };
    commandList.SetDescriptorHeaps(_countof(heaps), heaps);

    const D3D12_GPU_VIRTUAL_ADDRESS blurCBufferGpuVAddress(
        mBlurUploadCBuffer->GetResource().GetGPUVirtualAddress());

    commandList.SetGraphicsRootSignature(sRootSignature);
    commandList.SetGraphicsRootConstantBufferView(0U, blurCBufferGpuVAddress);
    commandList.SetGraphicsRootDescriptorTable(1U, mAmbientAccessibilityBufferShaderResourceView);

    commandList.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList.DrawInstanced(6U, 1U, 0U, 0U);

    commandList.Close();
    CommandListExecutor::Get().PushCommandList(commandList);

    return 1U;
}

bool
BlurCommandListRecorder::IsDataValid() const noexcept
{
    const bool result =
        mAmbientAccessibilityBufferShaderResourceView.ptr != 0UL &&
        mOutputAmbientAccessibilityBufferRenderTargetView.ptr != 0UL;

    return result;
}

void
BlurCommandListRecorder::InitBlurCBuffer() noexcept
{
    const std::size_t blurUploadCBufferElemSize =
        UploadBuffer::GetRoundedConstantBufferSizeInBytes(sizeof(BlurCBuffer));

    mBlurUploadCBuffer = &UploadBufferManager::CreateUploadBuffer(blurUploadCBufferElemSize,
                                                                  1U);
    BlurCBuffer blurCBuffer(AmbientOcclusionSettings::sNoiseTextureDimension);

    mBlurUploadCBuffer->CopyData(0U, &blurCBuffer, sizeof(BlurCBuffer));
}
}
#include "EnvironmentLightCommandListRecorder.h"

#include <DirectXMath.h>

#include <CommandListExecutor\CommandListExecutor.h>
#include <DescriptorManager\CbvSrvUavDescriptorManager.h>
#include <PSOManager/PSOManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <ShaderManager\ShaderManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils/DebugUtils.h>

namespace BRE {
// Root Signature:
// "CBV(b0, visibility = SHADER_VISIBILITY_VERTEX), " \ 0 -> Frame CBuffer
// "CBV(b0, visibility = SHADER_VISIBILITY_PIXEL), " \ 1 -> Frame CBuffer
// "DescriptorTable(SRV(t0), SRV(t1), SRV(t2), SRV(t3), SRV(t4), visibility = SHADER_VISIBILITY_PIXEL), " \ 2 -> Textures 
// "DescriptorTable(SRV(t5), visibility = SHADER_VISIBILITY_PIXEL), " \ 3 -> Depth buffer
namespace {
ID3D12PipelineState* sPSO{ nullptr };
ID3D12RootSignature* sRootSignature{ nullptr };
}

void
EnvironmentLightCommandListRecorder::InitSharedPSOAndRootSignature() noexcept
{
    BRE_ASSERT(sPSO == nullptr);
    BRE_ASSERT(sRootSignature == nullptr);

    PSOManager::PSOCreationData psoData{};
    psoData.mBlendDescriptor = D3DFactory::GetAlwaysBlendDesc();
    psoData.mDepthStencilDescriptor = D3DFactory::GetDisabledDepthStencilDesc();

    psoData.mPixelShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("EnvironmentLightPass/Shaders/EnvironmentLight/PS.cso");
    psoData.mVertexShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("EnvironmentLightPass/Shaders/EnvironmentLight/VS.cso");

    ID3DBlob* rootSignatureBlob = &ShaderManager::LoadShaderFileAndGetBlob("EnvironmentLightPass/Shaders/EnvironmentLight/RS.cso");
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
EnvironmentLightCommandListRecorder::Init(ID3D12Resource& normalSmoothnessBuffer,
                                          ID3D12Resource& baseColorMetalMaskBuffer,
                                          ID3D12Resource& diffuseIrradianceCubeMap,
                                          ID3D12Resource& specularPreConvolvedCubeMap,
                                          ID3D12Resource& ambientAccessibilityBuffer,
                                          const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferRenderTargetView,
                                          const D3D12_GPU_DESCRIPTOR_HANDLE& depthBufferShaderResourceView) noexcept
{
    BRE_ASSERT(IsDataValid() == false);

    mOutputColorBufferRenderTargetView = outputColorBufferRenderTargetView;
    mDepthBufferShaderResourceView = depthBufferShaderResourceView;

    InitShaderResourceViews(normalSmoothnessBuffer,
                            baseColorMetalMaskBuffer,
                            diffuseIrradianceCubeMap,
                            specularPreConvolvedCubeMap,
                            ambientAccessibilityBuffer);

    BRE_ASSERT(IsDataValid());
}

std::uint32_t
EnvironmentLightCommandListRecorder::RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept
{
    BRE_ASSERT(IsDataValid());
    BRE_ASSERT(sPSO != nullptr);
    BRE_ASSERT(sRootSignature != nullptr);

    // Update frame constants
    UploadBuffer& uploadFrameCBuffer(mFrameUploadCBufferPerFrame.GetNextFrameCBuffer());
    uploadFrameCBuffer.CopyData(0U, &frameCBuffer, sizeof(frameCBuffer));

    ID3D12GraphicsCommandList& commandList = mCommandListPerFrame.ResetCommandListWithNextCommandAllocator(sPSO);

    commandList.RSSetViewports(1U, &ApplicationSettings::sScreenViewport);
    commandList.RSSetScissorRects(1U, &ApplicationSettings::sScissorRect);
    commandList.OMSetRenderTargets(1U, &mOutputColorBufferRenderTargetView, false, nullptr);

    ID3D12DescriptorHeap* heaps[] = { &CbvSrvUavDescriptorManager::GetDescriptorHeap() };
    commandList.SetDescriptorHeaps(_countof(heaps), heaps);

    commandList.SetGraphicsRootSignature(sRootSignature);
    const D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(uploadFrameCBuffer.GetResource().GetGPUVirtualAddress());
    commandList.SetGraphicsRootConstantBufferView(0U, frameCBufferGpuVAddress);
    commandList.SetGraphicsRootConstantBufferView(1U, frameCBufferGpuVAddress);
    commandList.SetGraphicsRootDescriptorTable(2U, mPixelShaderResourceViewsBegin);
    commandList.SetGraphicsRootDescriptorTable(3U, mDepthBufferShaderResourceView);

    commandList.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList.DrawInstanced(6U, 1U, 0U, 0U);

    commandList.Close();
    CommandListExecutor::Get().PushCommandList(commandList);

    return 1U;
}

bool
EnvironmentLightCommandListRecorder::IsDataValid() const noexcept
{
    const bool result =
        mOutputColorBufferRenderTargetView.ptr != 0UL &&
        mDepthBufferShaderResourceView.ptr != 0UL &&
        mPixelShaderResourceViewsBegin.ptr != 0UL;

    return result;
}

void
EnvironmentLightCommandListRecorder::InitShaderResourceViews(ID3D12Resource& normalSmoothnessBuffer,
                                                             ID3D12Resource& baseColorMetalMaskBuffer,
                                                             ID3D12Resource& diffuseIrradianceCubeMap,
                                                             ID3D12Resource& specularPreConvolvedCubeMap,
                                                             ID3D12Resource& ambientAccessibilityBuffer) noexcept
{
    ID3D12Resource* resources[] =
    {
        &normalSmoothnessBuffer,
        &baseColorMetalMaskBuffer,
        &diffuseIrradianceCubeMap,
        &specularPreConvolvedCubeMap,
        &ambientAccessibilityBuffer,
    };

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptors[_countof(resources)]{};

    // Normal and smoothness geometry buffer
    srvDescriptors[0].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptors[0].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescriptors[0].Texture2D.MostDetailedMip = 0;
    srvDescriptors[0].Texture2D.ResourceMinLODClamp = 0.0f;
    srvDescriptors[0].Format = normalSmoothnessBuffer.GetDesc().Format;
    srvDescriptors[0].Texture2D.MipLevels = normalSmoothnessBuffer.GetDesc().MipLevels;
    
    // Base color and metal mask geometry buffer
    srvDescriptors[1].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptors[1].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescriptors[1].Texture2D.MostDetailedMip = 0;
    srvDescriptors[1].Texture2D.ResourceMinLODClamp = 0.0f;
    srvDescriptors[1].Format = baseColorMetalMaskBuffer.GetDesc().Format;
    srvDescriptors[1].Texture2D.MipLevels = baseColorMetalMaskBuffer.GetDesc().MipLevels;
   
    // Fill cube map texture descriptors	
    srvDescriptors[2].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptors[2].ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDescriptors[2].TextureCube.MostDetailedMip = 0;
    srvDescriptors[2].TextureCube.MipLevels = diffuseIrradianceCubeMap.GetDesc().MipLevels;
    srvDescriptors[2].TextureCube.ResourceMinLODClamp = 0.0f;
    srvDescriptors[2].Format = diffuseIrradianceCubeMap.GetDesc().Format;
    
    srvDescriptors[3].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptors[3].ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDescriptors[3].TextureCube.MostDetailedMip = 0;
    srvDescriptors[3].TextureCube.MipLevels = specularPreConvolvedCubeMap.GetDesc().MipLevels;
    srvDescriptors[3].TextureCube.ResourceMinLODClamp = 0.0f;
    srvDescriptors[3].Format = specularPreConvolvedCubeMap.GetDesc().Format;
    
    // Ambient accessibility buffer
    srvDescriptors[4].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptors[4].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescriptors[4].Texture2D.MostDetailedMip = 0;
    srvDescriptors[4].Texture2D.ResourceMinLODClamp = 0.0f;
    srvDescriptors[4].Format = ambientAccessibilityBuffer.GetDesc().Format;
    srvDescriptors[4].Texture2D.MipLevels = ambientAccessibilityBuffer.GetDesc().MipLevels;

    BRE_ASSERT(_countof(resources) == _countof(srvDescriptors));
    
    mPixelShaderResourceViewsBegin = CbvSrvUavDescriptorManager::CreateShaderResourceViews(resources,
                                                                                           srvDescriptors,
                                                                                           _countof(srvDescriptors));
}
}
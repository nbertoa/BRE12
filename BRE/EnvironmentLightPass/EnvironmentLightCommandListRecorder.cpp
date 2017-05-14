#include "EnvironmentLightCommandListRecorder.h"

#include <DirectXMath.h>

#include <CommandListExecutor\CommandListExecutor.h>
#include <DescriptorManager\CbvSrvUavDescriptorManager.h>
#include <PSOManager/PSOManager.h>
#include <ResourceManager/UploadBufferManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <ShaderManager\ShaderManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils/DebugUtils.h>

namespace BRE {
// Root Signature:
// "CBV(b0, visibility = SHADER_VISIBILITY_VERTEX), " \ 0 -> Frame CBuffer
// "CBV(b0, visibility = SHADER_VISIBILITY_PIXEL), " \ 1 -> Frame CBuffer
// "DescriptorTable(SRV(t0), SRV(t1), SRV(t2), SRV(t3), SRV(t4), SRV(t5), visibility = SHADER_VISIBILITY_PIXEL), " \ 2 -> Textures 

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
    psoData.mRenderTargetFormats[0U] = SettingsManager::sColorBufferFormat;
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
                                      ID3D12Resource& depthBuffer,
                                      ID3D12Resource& diffuseIrradianceCubeMap,
                                      ID3D12Resource& specularPreConvolvedCubeMap,
                                      ID3D12Resource& ambientAccessibilityBuffer,
                                      const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept
{
    BRE_ASSERT(ValidateData() == false);

    mRenderTargetView = renderTargetView;

    InitShaderResourceViews(normalSmoothnessBuffer,
                            baseColorMetalMaskBuffer,
                            depthBuffer,
                            diffuseIrradianceCubeMap,
                            ambientAccessibilityBuffer,
                            specularPreConvolvedCubeMap);

    BRE_ASSERT(ValidateData());
}

void
EnvironmentLightCommandListRecorder::RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept
{
    BRE_ASSERT(ValidateData());
    BRE_ASSERT(sPSO != nullptr);
    BRE_ASSERT(sRootSignature != nullptr);

    // Update frame constants
    UploadBuffer& uploadFrameCBuffer(mFrameUploadCBufferPerFrame.GetNextFrameCBuffer());
    uploadFrameCBuffer.CopyData(0U, &frameCBuffer, sizeof(frameCBuffer));

    ID3D12GraphicsCommandList& commandList = mCommandListPerFrame.ResetCommandListWithNextCommandAllocator(sPSO);

    commandList.RSSetViewports(1U, &SettingsManager::sScreenViewport);
    commandList.RSSetScissorRects(1U, &SettingsManager::sScissorRect);
    commandList.OMSetRenderTargets(1U, &mRenderTargetView, false, nullptr);

    ID3D12DescriptorHeap* heaps[] = { &CbvSrvUavDescriptorManager::GetDescriptorHeap() };
    commandList.SetDescriptorHeaps(_countof(heaps), heaps);

    commandList.SetGraphicsRootSignature(sRootSignature);
    const D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(uploadFrameCBuffer.GetResource()->GetGPUVirtualAddress());
    commandList.SetGraphicsRootConstantBufferView(0U, frameCBufferGpuVAddress);
    commandList.SetGraphicsRootConstantBufferView(1U, frameCBufferGpuVAddress);
    commandList.SetGraphicsRootDescriptorTable(2U, mStartPixelShaderResourceView);

    commandList.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList.DrawInstanced(6U, 1U, 0U, 0U);

    commandList.Close();
    CommandListExecutor::Get().AddCommandList(commandList);
}

bool
EnvironmentLightCommandListRecorder::ValidateData() const noexcept
{
    const bool result =
        mRenderTargetView.ptr != 0UL &&
        mStartPixelShaderResourceView.ptr != 0UL;

    return result;
}

void
EnvironmentLightCommandListRecorder::InitShaderResourceViews(ID3D12Resource& normalSmoothnessBuffer,
                                                         ID3D12Resource& baseColorMetalMaskBuffer,
                                                         ID3D12Resource& depthBuffer,
                                                         ID3D12Resource& diffuseIrradianceCubeMap,
                                                         ID3D12Resource& ambientAccessibilityBuffer,
                                                         ID3D12Resource& specularPreConvolvedCubeMap) noexcept
{
    // Number of geometry buffers + depth buffer + 2 cube maps + ambient accessibility buffer
    const std::uint32_t numResources = 6U;

    std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> srvDescriptors;
    srvDescriptors.reserve(numResources);

    std::vector<ID3D12Resource*> resources;
    resources.reserve(numResources);

    // Normal and smoothness geometry buffer
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptor{};
    srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescriptor.Texture2D.MostDetailedMip = 0;
    srvDescriptor.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDescriptor.Format = normalSmoothnessBuffer.GetDesc().Format;
    srvDescriptor.Texture2D.MipLevels = normalSmoothnessBuffer.GetDesc().MipLevels;
    srvDescriptors.emplace_back(srvDescriptor);
    resources.push_back(&normalSmoothnessBuffer);

    // Base color and metal mask geometry buffer
    srvDescriptor = D3D12_SHADER_RESOURCE_VIEW_DESC{};
    srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescriptor.Texture2D.MostDetailedMip = 0;
    srvDescriptor.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDescriptor.Format = baseColorMetalMaskBuffer.GetDesc().Format;
    srvDescriptor.Texture2D.MipLevels = baseColorMetalMaskBuffer.GetDesc().MipLevels;
    srvDescriptors.emplace_back(srvDescriptor);
    resources.push_back(&baseColorMetalMaskBuffer);

    // Fill depth buffer descriptor
    srvDescriptor = D3D12_SHADER_RESOURCE_VIEW_DESC{};
    srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescriptor.Texture2D.MostDetailedMip = 0;
    srvDescriptor.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDescriptor.Format = SettingsManager::sDepthStencilSRVFormat;
    srvDescriptor.Texture2D.MipLevels = depthBuffer.GetDesc().MipLevels;
    srvDescriptors.emplace_back(srvDescriptor);
    resources.push_back(&depthBuffer);

    // Fill cube map texture descriptors	
    srvDescriptor = D3D12_SHADER_RESOURCE_VIEW_DESC{};
    srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDescriptor.TextureCube.MostDetailedMip = 0;
    srvDescriptor.TextureCube.MipLevels = diffuseIrradianceCubeMap.GetDesc().MipLevels;
    srvDescriptor.TextureCube.ResourceMinLODClamp = 0.0f;
    srvDescriptor.Format = diffuseIrradianceCubeMap.GetDesc().Format;
    srvDescriptors.emplace_back(srvDescriptor);
    resources.push_back(&diffuseIrradianceCubeMap);

    srvDescriptor = D3D12_SHADER_RESOURCE_VIEW_DESC{};
    srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDescriptor.TextureCube.MostDetailedMip = 0;
    srvDescriptor.TextureCube.MipLevels = specularPreConvolvedCubeMap.GetDesc().MipLevels;
    srvDescriptor.TextureCube.ResourceMinLODClamp = 0.0f;
    srvDescriptor.Format = specularPreConvolvedCubeMap.GetDesc().Format;
    srvDescriptors.emplace_back(srvDescriptor);
    resources.push_back(&specularPreConvolvedCubeMap);

    // Ambient accessibility buffer
    srvDescriptor = D3D12_SHADER_RESOURCE_VIEW_DESC{};
    srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescriptor.Texture2D.MostDetailedMip = 0;
    srvDescriptor.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDescriptor.Format = ambientAccessibilityBuffer.GetDesc().Format;
    srvDescriptor.Texture2D.MipLevels = ambientAccessibilityBuffer.GetDesc().MipLevels;
    srvDescriptors.emplace_back(srvDescriptor);
    resources.push_back(&ambientAccessibilityBuffer);

    mStartPixelShaderResourceView = CbvSrvUavDescriptorManager::CreateShaderResourceViews(resources.data(),
                                                                                          srvDescriptors.data(),
                                                                                          numResources);
}
}


#include "ColorHeightMappingCommandListRecorder.h"

#include <DirectXMath.h>

#include <CommandListExecutor\CommandListExecutor.h>
#include <DescriptorManager\CbvSrvUavDescriptorManager.h>
#include <DirectXManager\DirectXManager.h>
#include <GeometryPass\GeometrySettings.h>
#include <GeometryPass\Shaders\HeightMappingCBuffer.h>
#include <MathUtils/MathUtils.h>
#include <PSOManager/PSOManager.h>
#include <ResourceManager/UploadBufferManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <ShaderManager\ShaderManager.h>
#include <ShaderUtils\CBuffers.h>
#include <ShaderUtils\MaterialProperties.h>
#include <Utils/DebugUtils.h>

namespace BRE {
// Root signature:
// "DescriptorTable(CBV(b0), visibility = SHADER_VISIBILITY_VERTEX), " \ 0 -> Object CBuffers
// "CBV(b1, visibility = SHADER_VISIBILITY_VERTEX), " \ 1 -> Frame CBuffer
// "CBV(b2, visibility = SHADER_VISIBILITY_VERTEX), " \ 2 -> Height Mapping CBuffer
// "CBV(b0, visibility = SHADER_VISIBILITY_DOMAIN), " \ 3 -> Frame CBuffer
// "CBV(b1, visibility = SHADER_VISIBILITY_DOMAIN), " \ 4 -> Height Mapping CBuffer
// "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_DOMAIN), " \ 5 -> Height Texture
// "DescriptorTable(CBV(b0), visibility = SHADER_VISIBILITY_PIXEL), " \ 6 -> Material CBuffers
// "CBV(b1, visibility = SHADER_VISIBILITY_PIXEL), " \ 7 -> Frame CBuffer
// "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL), " \ 8 -> Normal Texture

namespace {
ID3D12PipelineState* sPSO{ nullptr };
ID3D12RootSignature* sRootSignature{ nullptr };
}

void
ColorHeightMappingCommandListRecorder::InitSharedPSOAndRootSignature(const DXGI_FORMAT* geometryBufferFormats,
                                                                     const std::uint32_t geometryBufferCount) noexcept
{
    BRE_ASSERT(geometryBufferFormats != nullptr);
    BRE_ASSERT(geometryBufferCount > 0U);
    BRE_ASSERT(sPSO == nullptr);
    BRE_ASSERT(sRootSignature == nullptr);

    // Build pso and root signature
    PSOManager::PSOCreationData psoData{};
    psoData.mInputLayoutDescriptors = D3DFactory::GetPositionNormalTangentTexCoordInputLayout();

    psoData.mDomainShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("GeometryPass/Shaders/ColorHeightMapping/DS.cso");
    psoData.mHullShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("GeometryPass/Shaders/ColorHeightMapping/HS.cso");
    psoData.mPixelShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("GeometryPass/Shaders/ColorHeightMapping/PS.cso");
    psoData.mVertexShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("GeometryPass/Shaders/ColorHeightMapping/VS.cso");

    ID3DBlob* rootSignatureBlob = &ShaderManager::LoadShaderFileAndGetBlob("GeometryPass/Shaders/ColorHeightMapping/RS.cso");
    psoData.mRootSignature = &RootSignatureManager::CreateRootSignatureFromBlob(*rootSignatureBlob);
    sRootSignature = psoData.mRootSignature;

    psoData.mPrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    psoData.mNumRenderTargets = geometryBufferCount;
    memcpy(psoData.mRenderTargetFormats, geometryBufferFormats, sizeof(DXGI_FORMAT) * psoData.mNumRenderTargets);
    sPSO = &PSOManager::CreateGraphicsPSO(psoData);

    BRE_ASSERT(sPSO != nullptr);
    BRE_ASSERT(sRootSignature != nullptr);
}

void
ColorHeightMappingCommandListRecorder::Init(const std::vector<GeometryData>& geometryDataVector,
                                            const std::vector<MaterialProperties>& materialProperties,
                                            const std::vector<ID3D12Resource*>& normalTextures,
                                            const std::vector<ID3D12Resource*>& heightTextures) noexcept
{
    BRE_ASSERT(IsDataValid() == false);
    BRE_ASSERT(geometryDataVector.empty() == false);
    BRE_ASSERT(materialProperties.empty() == false);
    BRE_ASSERT(materialProperties.size() == normalTextures.size());
    BRE_ASSERT(normalTextures.size() == heightTextures.size());

    const std::size_t numResources = materialProperties.size();
    const std::size_t geometryDataCount = geometryDataVector.size();

    // Check that the total number of matrices (geometry to be drawn) will be equal to available materials
#ifdef _DEBUG
    std::size_t totalNumMatrices{ 0UL };
    for (std::size_t i = 0UL; i < geometryDataCount; ++i) {
        const std::size_t numMatrices{ geometryDataVector[i].mWorldMatrices.size() };
        totalNumMatrices += numMatrices;
        BRE_ASSERT(numMatrices != 0UL);
    }
    BRE_ASSERT(totalNumMatrices == numResources);
#endif
    mGeometryDataVec.reserve(geometryDataCount);
    for (std::uint32_t i = 0U; i < geometryDataCount; ++i) {
        mGeometryDataVec.push_back(geometryDataVector[i]);
    }

    InitCBuffersAndViews(materialProperties,
                         normalTextures,
                         heightTextures);

    BRE_ASSERT(IsDataValid());
}

std::uint32_t
ColorHeightMappingCommandListRecorder::RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept
{
    BRE_ASSERT(IsDataValid());
    BRE_ASSERT(sPSO != nullptr);
    BRE_ASSERT(sRootSignature != nullptr);
    BRE_ASSERT(mGeometryBufferRenderTargetViews != nullptr);
    BRE_ASSERT(mGeometryBufferRenderTargetViewCount != 0U);
    BRE_ASSERT(mDepthBufferView.ptr != 0U);

    // Update frame constants
    UploadBuffer& uploadFrameCBuffer(mFrameUploadCBufferPerFrame.GetNextFrameCBuffer());
    uploadFrameCBuffer.CopyData(0U, &frameCBuffer, sizeof(frameCBuffer));

    ID3D12GraphicsCommandList& commandList = mCommandListPerFrame.ResetCommandListWithNextCommandAllocator(sPSO);

    commandList.RSSetViewports(1U, &ApplicationSettings::sScreenViewport);
    commandList.RSSetScissorRects(1U, &ApplicationSettings::sScissorRect);
    commandList.OMSetRenderTargets(mGeometryBufferRenderTargetViewCount,
                                   mGeometryBufferRenderTargetViews,
                                   false,
                                   &mDepthBufferView);

    ID3D12DescriptorHeap* heaps[] = { &CbvSrvUavDescriptorManager::GetDescriptorHeap() };
    commandList.SetDescriptorHeaps(_countof(heaps), heaps);
    commandList.SetGraphicsRootSignature(sRootSignature);

    const std::size_t descHandleIncSize{ DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
    D3D12_GPU_DESCRIPTOR_HANDLE objectCBufferView(mObjectCBufferViewsBegin);
    D3D12_GPU_DESCRIPTOR_HANDLE materialCBufferView(mMaterialCBufferViewsBegin);
    D3D12_GPU_DESCRIPTOR_HANDLE normalTextureRenderTargetView(mNormalTextureRenderTargetViewsBegin);
    D3D12_GPU_DESCRIPTOR_HANDLE heightTextureRenderTargetView(mHeightTextureRenderTargetViewsBegin);

    commandList.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

    // Set frame constants root parameters
    const D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(
        uploadFrameCBuffer.GetResource().GetGPUVirtualAddress());
    const D3D12_GPU_VIRTUAL_ADDRESS heightMappingCBufferGpuVAddress(
        mHeightMappingUploadCBuffer->GetResource().GetGPUVirtualAddress());
    commandList.SetGraphicsRootConstantBufferView(1U, frameCBufferGpuVAddress);
    commandList.SetGraphicsRootConstantBufferView(2U, heightMappingCBufferGpuVAddress);
    commandList.SetGraphicsRootConstantBufferView(3U, frameCBufferGpuVAddress);
    commandList.SetGraphicsRootConstantBufferView(4U, heightMappingCBufferGpuVAddress);
    commandList.SetGraphicsRootConstantBufferView(7U, frameCBufferGpuVAddress);

    // Draw objects
    const std::size_t geomCount{ mGeometryDataVec.size() };
    for (std::size_t i = 0UL; i < geomCount; ++i) {
        GeometryData& geomData{ mGeometryDataVec[i] };
        commandList.IASetVertexBuffers(0U, 1U, &geomData.mVertexBufferData.mBufferView);
        commandList.IASetIndexBuffer(&geomData.mIndexBufferData.mBufferView);
        const std::size_t worldMatsCount{ geomData.mWorldMatrices.size() };
        for (std::size_t j = 0UL; j < worldMatsCount; ++j) {
            commandList.SetGraphicsRootDescriptorTable(0U, objectCBufferView);
            objectCBufferView.ptr += descHandleIncSize;

            commandList.SetGraphicsRootDescriptorTable(5U, heightTextureRenderTargetView);
            heightTextureRenderTargetView.ptr += descHandleIncSize;

            commandList.SetGraphicsRootDescriptorTable(6U, materialCBufferView);
            materialCBufferView.ptr += descHandleIncSize;

            commandList.SetGraphicsRootDescriptorTable(8U, normalTextureRenderTargetView);
            normalTextureRenderTargetView.ptr += descHandleIncSize;

            commandList.DrawIndexedInstanced(geomData.mIndexBufferData.mElementCount, 1U, 0U, 0U, 0U);
        }
    }

    commandList.Close();

    CommandListExecutor::Get().PushCommandList(commandList);

    return 1U;
}

bool
ColorHeightMappingCommandListRecorder::IsDataValid() const noexcept
{
    const bool result =
        mNormalTextureRenderTargetViewsBegin.ptr != 0UL &&
        mHeightTextureRenderTargetViewsBegin.ptr != 0UL &&
        mHeightMappingUploadCBuffer != nullptr;

    return result;
}

void
ColorHeightMappingCommandListRecorder::InitCBuffersAndViews(const std::vector<MaterialProperties>& materialProperties,
                                                            const std::vector<ID3D12Resource*>& normalTextures,
                                                            const std::vector<ID3D12Resource*>& heightTextures) noexcept
{
    BRE_ASSERT(materialProperties.empty() == false);
    BRE_ASSERT(materialProperties.size() == normalTextures.size());
    BRE_ASSERT(normalTextures.size() == heightTextures.size());
    BRE_ASSERT(mObjectUploadCBuffers == nullptr);
    BRE_ASSERT(mMaterialUploadCBuffers == nullptr);

    const std::uint32_t numResources = static_cast<std::uint32_t>(materialProperties.size());

    // Create object cbuffer and fill it
    const std::size_t objCBufferElemSize{ UploadBuffer::GetRoundedConstantBufferSizeInBytes(sizeof(ObjectCBuffer)) };
    mObjectUploadCBuffers = &UploadBufferManager::CreateUploadBuffer(objCBufferElemSize, numResources);
    std::uint32_t k = 0U;
    const std::size_t geometryDataCount{ mGeometryDataVec.size() };
    ObjectCBuffer objCBuffer;
    for (std::size_t i = 0UL; i < geometryDataCount; ++i) {
        GeometryData& geomData{ mGeometryDataVec[i] };
        const std::uint32_t worldMatsCount{ static_cast<std::uint32_t>(geomData.mWorldMatrices.size()) };
        for (std::uint32_t j = 0UL; j < worldMatsCount; ++j) {
            MathUtils::StoreTransposeMatrix(geomData.mWorldMatrices[j], 
                                            objCBuffer.mWorldMatrix);
            MathUtils::StoreTransposeMatrix(geomData.mInverseTransposeWorldMatrices[j],
                                            objCBuffer.mInverseTransposeWorldMatrix);
            mObjectUploadCBuffers->CopyData(k + j, &objCBuffer, sizeof(objCBuffer));
        }

        k += worldMatsCount;
    }

    // Create materials cbuffer		
    const std::size_t matCBufferElemSize{ UploadBuffer::GetRoundedConstantBufferSizeInBytes(sizeof(MaterialProperties)) };
    mMaterialUploadCBuffers = &UploadBufferManager::CreateUploadBuffer(matCBufferElemSize, numResources);

    D3D12_GPU_VIRTUAL_ADDRESS materialsGpuAddress{ mMaterialUploadCBuffers->GetResource().GetGPUVirtualAddress() };
    D3D12_GPU_VIRTUAL_ADDRESS objCBufferGpuAddress{ mObjectUploadCBuffers->GetResource().GetGPUVirtualAddress() };

    // Create object / materials cbuffers descriptors
    // Create textures SRV descriptors
    std::vector<D3D12_CONSTANT_BUFFER_VIEW_DESC> objectCbufferViewDescVec;
    objectCbufferViewDescVec.reserve(numResources);

    std::vector<D3D12_CONSTANT_BUFFER_VIEW_DESC> materialCbufferViewDescVec;
    materialCbufferViewDescVec.reserve(numResources);

    std::vector<ID3D12Resource*> normalResVec;
    normalResVec.reserve(numResources);
    std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> normalSrvDescVec;
    normalSrvDescVec.reserve(numResources);

    std::vector<ID3D12Resource*> heightResVec;
    heightResVec.reserve(numResources);
    std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> heightSrvDescVec;
    heightSrvDescVec.reserve(numResources);
    for (std::size_t i = 0UL; i < numResources; ++i) {
        // Object cbuffer desc
        D3D12_CONSTANT_BUFFER_VIEW_DESC cBufferDesc{};
        cBufferDesc.BufferLocation = objCBufferGpuAddress + i * objCBufferElemSize;
        cBufferDesc.SizeInBytes = static_cast<std::uint32_t>(objCBufferElemSize);
        objectCbufferViewDescVec.push_back(cBufferDesc);

        // Material cbuffer desc
        cBufferDesc.BufferLocation = materialsGpuAddress + i * matCBufferElemSize;
        cBufferDesc.SizeInBytes = static_cast<std::uint32_t>(matCBufferElemSize);
        materialCbufferViewDescVec.push_back(cBufferDesc);

        // Normal descriptor
        normalResVec.push_back(normalTextures[i]);
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptor{};
        srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDescriptor.Texture2D.MostDetailedMip = 0;
        srvDescriptor.Texture2D.ResourceMinLODClamp = 0.0f;
        srvDescriptor.Format = normalResVec.back()->GetDesc().Format;
        srvDescriptor.Texture2D.MipLevels = normalResVec.back()->GetDesc().MipLevels;
        normalSrvDescVec.push_back(srvDescriptor);

        // Height descriptor
        heightResVec.push_back(heightTextures[i]);
        srvDescriptor = D3D12_SHADER_RESOURCE_VIEW_DESC{};
        srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDescriptor.Texture2D.MostDetailedMip = 0;
        srvDescriptor.Texture2D.ResourceMinLODClamp = 0.0f;
        srvDescriptor.Format = heightResVec.back()->GetDesc().Format;
        srvDescriptor.Texture2D.MipLevels = heightResVec.back()->GetDesc().MipLevels;
        heightSrvDescVec.push_back(srvDescriptor);

        mMaterialUploadCBuffers->CopyData(static_cast<std::uint32_t>(i), &materialProperties[i], sizeof(MaterialProperties));
    }
    mObjectCBufferViewsBegin =
        CbvSrvUavDescriptorManager::CreateConstantBufferViews(objectCbufferViewDescVec.data(),
                                                              static_cast<std::uint32_t>(objectCbufferViewDescVec.size()));
    mMaterialCBufferViewsBegin =
        CbvSrvUavDescriptorManager::CreateConstantBufferViews(materialCbufferViewDescVec.data(),
                                                              static_cast<std::uint32_t>(materialCbufferViewDescVec.size()));
    mNormalTextureRenderTargetViewsBegin =
        CbvSrvUavDescriptorManager::CreateShaderResourceViews(normalResVec.data(),
                                                              normalSrvDescVec.data(),
                                                              static_cast<std::uint32_t>(normalSrvDescVec.size()));
    mHeightTextureRenderTargetViewsBegin =
        CbvSrvUavDescriptorManager::CreateShaderResourceViews(heightResVec.data(),
                                                              heightSrvDescVec.data(),
                                                              static_cast<std::uint32_t>(heightSrvDescVec.size()));

    // Height mapping constant buffer
    const std::size_t heightMappingUploadCBufferElemSize = 
        UploadBuffer::GetRoundedConstantBufferSizeInBytes(sizeof(HeightMappingCBuffer));

    mHeightMappingUploadCBuffer = &UploadBufferManager::CreateUploadBuffer(heightMappingUploadCBufferElemSize,
                                                                           1U);
    HeightMappingCBuffer heightMappingCBuffer(GeometrySettings::sMinTessellationDistance,
                                              GeometrySettings::sMaxTessellationDistance,
                                              GeometrySettings::sMinTessellationFactor,
                                              GeometrySettings::sMaxTessellationFactor,
                                              GeometrySettings::sHeightScale);

    mHeightMappingUploadCBuffer->CopyData(0U, &heightMappingCBuffer, sizeof(HeightMappingCBuffer));
}
}
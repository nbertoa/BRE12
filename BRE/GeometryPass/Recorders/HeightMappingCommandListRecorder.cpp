#include "HeightMappingCommandListRecorder.h"

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
#include <Utils/DebugUtils.h>

namespace BRE {
// Root signature:
// "DescriptorTable(CBV(b0), visibility = SHADER_VISIBILITY_VERTEX), " \ 0 -> Object CBuffers
// "CBV(b1, visibility = SHADER_VISIBILITY_VERTEX), " \ 1 -> Frame CBuffer
// "CBV(b2, visibility = SHADER_VISIBILITY_VERTEX), " \ 2 -> Height Mapping CBuffer
// "CBV(b0, visibility = SHADER_VISIBILITY_DOMAIN), " \ 3 -> Frame CBuffer
// "CBV(b1, visibility = SHADER_VISIBILITY_DOMAIN), " \ 4 -> Height Mapping CBuffer
// "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_DOMAIN), " \ 5 -> Height Texture
// "CBV(b0, visibility = SHADER_VISIBILITY_PIXEL), " \ 6 -> Frame CBuffer
// "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL), " \ 7 -> Base Color Texture
// "DescriptorTable(SRV(t1), visibility = SHADER_VISIBILITY_PIXEL), " \ 8 -> Metalness Texture
// "DescriptorTable(SRV(t2), visibility = SHADER_VISIBILITY_PIXEL), " \ 9 -> Roughness Texture
// "DescriptorTable(SRV(t3), visibility = SHADER_VISIBILITY_PIXEL), " \ 10 -> Normal Texture

namespace {
ID3D12PipelineState* sPSO{ nullptr };
ID3D12RootSignature* sRootSignature{ nullptr };
}

void
HeightMappingCommandListRecorder::InitSharedPSOAndRootSignature(const DXGI_FORMAT* geometryBufferFormats,
                                                                const std::uint32_t geometryBufferCount) noexcept
{
    BRE_ASSERT(geometryBufferFormats != nullptr);
    BRE_ASSERT(geometryBufferCount > 0U);
    BRE_ASSERT(sPSO == nullptr);
    BRE_ASSERT(sRootSignature == nullptr);

    // Build pso and root signature
    PSOManager::PSOCreationData psoData{};
    psoData.mInputLayoutDescriptors = D3DFactory::GetPositionNormalTangentTexCoordInputLayout();

    psoData.mDomainShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("GeometryPass/Shaders/HeightMapping/DS.cso");
    psoData.mHullShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("GeometryPass/Shaders/HeightMapping/HS.cso");
    psoData.mPixelShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("GeometryPass/Shaders/HeightMapping/PS.cso");
    psoData.mVertexShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("GeometryPass/Shaders/HeightMapping/VS.cso");

    ID3DBlob* rootSignatureBlob = &ShaderManager::LoadShaderFileAndGetBlob("GeometryPass/Shaders/HeightMapping/RS.cso");
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
HeightMappingCommandListRecorder::Init(const std::vector<GeometryData>& geometryDataVector,
                                       const std::vector<ID3D12Resource*>& baseColorTextures,
                                       const std::vector<ID3D12Resource*>& metalnessTextures,
                                       const std::vector<ID3D12Resource*>& roughnessTextures,
                                       const std::vector<ID3D12Resource*>& normalTextures,
                                       const std::vector<ID3D12Resource*>& heightTextures) noexcept
{
    BRE_ASSERT(IsDataValid() == false);
    BRE_ASSERT(geometryDataVector.empty() == false);
    BRE_ASSERT(baseColorTextures.empty() == false);
    BRE_ASSERT(baseColorTextures.size() == metalnessTextures.size());
    BRE_ASSERT(metalnessTextures.size() == roughnessTextures.size());
    BRE_ASSERT(roughnessTextures.size() == normalTextures.size());
    BRE_ASSERT(normalTextures.size() == heightTextures.size());

    const std::size_t numResources = baseColorTextures.size();
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

    InitCBuffersAndViews(baseColorTextures,
                         metalnessTextures,
                         roughnessTextures,
                         normalTextures,
                         heightTextures);

    BRE_ASSERT(IsDataValid());
}

std::uint32_t
HeightMappingCommandListRecorder::RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept
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
    D3D12_GPU_DESCRIPTOR_HANDLE baseColorTextureRenderTargetView(mBaseColorTextureRenderTargetViewsBegin);
    D3D12_GPU_DESCRIPTOR_HANDLE metalnessTextureRenderTargetView(mMetalnessTextureRenderTargetViewsBegin);
    D3D12_GPU_DESCRIPTOR_HANDLE roughnessTextureRenderTargetView(mRoughnessTextureRenderTargetViewsBegin);
    D3D12_GPU_DESCRIPTOR_HANDLE normalTextureRenderTargetView(mNormalTextureRenderTargetViewsBegin);
    D3D12_GPU_DESCRIPTOR_HANDLE heightTextureRenderTargetView(mHeightTextureRenderTargetViewsBegin);

    commandList.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

    // Set frame constants root parameters
    D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(
        uploadFrameCBuffer.GetResource().GetGPUVirtualAddress());
    const D3D12_GPU_VIRTUAL_ADDRESS heightMappingCBufferGpuVAddress(
        mHeightMappingUploadCBuffer->GetResource().GetGPUVirtualAddress());
    commandList.SetGraphicsRootConstantBufferView(1U, frameCBufferGpuVAddress);
    commandList.SetGraphicsRootConstantBufferView(2U, heightMappingCBufferGpuVAddress);
    commandList.SetGraphicsRootConstantBufferView(3U, frameCBufferGpuVAddress);
    commandList.SetGraphicsRootConstantBufferView(4U, heightMappingCBufferGpuVAddress);
    commandList.SetGraphicsRootConstantBufferView(6U, frameCBufferGpuVAddress);

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
            
            commandList.SetGraphicsRootDescriptorTable(7U, baseColorTextureRenderTargetView);
            baseColorTextureRenderTargetView.ptr += descHandleIncSize;

            commandList.SetGraphicsRootDescriptorTable(8U, metalnessTextureRenderTargetView);
            metalnessTextureRenderTargetView.ptr += descHandleIncSize;

            commandList.SetGraphicsRootDescriptorTable(9U, roughnessTextureRenderTargetView);
            roughnessTextureRenderTargetView.ptr += descHandleIncSize;

            commandList.SetGraphicsRootDescriptorTable(10U, normalTextureRenderTargetView);
            normalTextureRenderTargetView.ptr += descHandleIncSize;

            commandList.DrawIndexedInstanced(geomData.mIndexBufferData.mElementCount, 1U, 0U, 0U, 0U);
        }
    }

    commandList.Close();
    CommandListExecutor::Get().PushCommandList(commandList);

    return 1U;
}

bool
HeightMappingCommandListRecorder::IsDataValid() const noexcept
{
    const bool result =
        GeometryCommandListRecorder::IsDataValid() &&
        mBaseColorTextureRenderTargetViewsBegin.ptr != 0UL &&
        mMetalnessTextureRenderTargetViewsBegin.ptr != 0UL &&
        mRoughnessTextureRenderTargetViewsBegin.ptr != 0UL &&
        mNormalTextureRenderTargetViewsBegin.ptr != 0UL &&
        mHeightTextureRenderTargetViewsBegin.ptr != 0UL &&
        mHeightMappingUploadCBuffer != nullptr;

    return result;
}

void
HeightMappingCommandListRecorder::InitCBuffersAndViews(const std::vector<ID3D12Resource*>& baseColorTextures,
                                                       const std::vector<ID3D12Resource*>& metalnessTextures,
                                                       const std::vector<ID3D12Resource*>& roughnessTextures,
                                                       const std::vector<ID3D12Resource*>& normalTextures,
                                                       const std::vector<ID3D12Resource*>& heightTextures) noexcept
{
    BRE_ASSERT(baseColorTextures.empty() == false);
    BRE_ASSERT(baseColorTextures.size() == metalnessTextures.size());
    BRE_ASSERT(metalnessTextures.size() == roughnessTextures.size());
    BRE_ASSERT(roughnessTextures.size() == normalTextures.size());
    BRE_ASSERT(normalTextures.size() == heightTextures.size());
    BRE_ASSERT(mObjectUploadCBuffers == nullptr);

    const std::uint32_t numResources = static_cast<std::uint32_t>(baseColorTextures.size());

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
            objCBuffer.mTextureScale = geomData.mTextureScales[j];
            mObjectUploadCBuffers->CopyData(k + j, &objCBuffer, sizeof(objCBuffer));
        }

        k += worldMatsCount;
    }
    
    D3D12_GPU_VIRTUAL_ADDRESS objCBufferGpuAddress{ mObjectUploadCBuffers->GetResource().GetGPUVirtualAddress() };

    // Create object / materials cbuffers descriptors
    // Create textures SRV descriptors
    std::vector<D3D12_CONSTANT_BUFFER_VIEW_DESC> objectCbufferViewDescVec;
    objectCbufferViewDescVec.reserve(numResources);

    std::vector<D3D12_CONSTANT_BUFFER_VIEW_DESC> materialCbufferViewDescVec;
    materialCbufferViewDescVec.reserve(numResources);

    std::vector<ID3D12Resource*> textureResVec;
    textureResVec.reserve(numResources);
    std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> textureSrvDescVec;
    textureSrvDescVec.reserve(numResources);

    std::vector<ID3D12Resource*> metalnessResVec;
    metalnessResVec.reserve(numResources);
    std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> metalnessSrvDescVec;
    metalnessSrvDescVec.reserve(numResources);

    std::vector<ID3D12Resource*> roughnessResVec;
    roughnessResVec.reserve(numResources);
    std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> roughnessSrvDescVec;
    roughnessSrvDescVec.reserve(numResources);

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

        // Texture descriptor
        textureResVec.push_back(baseColorTextures[i]);
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        srvDesc.Format = textureResVec.back()->GetDesc().Format;
        srvDesc.Texture2D.MipLevels = textureResVec.back()->GetDesc().MipLevels;
        textureSrvDescVec.push_back(srvDesc);

        // Metalness descriptor
        metalnessResVec.push_back(metalnessTextures[i]);

        srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        srvDesc.Format = metalnessResVec.back()->GetDesc().Format;
        srvDesc.Texture2D.MipLevels = metalnessResVec.back()->GetDesc().MipLevels;
        metalnessSrvDescVec.push_back(srvDesc);

        // Roughness descriptor
        roughnessResVec.push_back(roughnessTextures[i]);

        srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        srvDesc.Format = roughnessResVec.back()->GetDesc().Format;
        srvDesc.Texture2D.MipLevels = roughnessResVec.back()->GetDesc().MipLevels;
        roughnessSrvDescVec.push_back(srvDesc);

        // Normal descriptor
        normalResVec.push_back(normalTextures[i]);
        srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        srvDesc.Format = normalResVec.back()->GetDesc().Format;
        srvDesc.Texture2D.MipLevels = normalResVec.back()->GetDesc().MipLevels;
        normalSrvDescVec.push_back(srvDesc);

        // Height descriptor
        heightResVec.push_back(heightTextures[i]);
        srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        srvDesc.Format = heightResVec.back()->GetDesc().Format;
        srvDesc.Texture2D.MipLevels = heightResVec.back()->GetDesc().MipLevels;
        heightSrvDescVec.push_back(srvDesc);
    }

    mObjectCBufferViewsBegin =
        CbvSrvUavDescriptorManager::CreateConstantBufferViews(objectCbufferViewDescVec.data(),
                                                              static_cast<std::uint32_t>(objectCbufferViewDescVec.size()));
                                                              
    mBaseColorTextureRenderTargetViewsBegin =
        CbvSrvUavDescriptorManager::CreateShaderResourceViews(textureResVec.data(),
                                                              textureSrvDescVec.data(),
                                                              static_cast<std::uint32_t>(textureSrvDescVec.size()));

    mMetalnessTextureRenderTargetViewsBegin =
        CbvSrvUavDescriptorManager::CreateShaderResourceViews(metalnessResVec.data(),
                                                              metalnessSrvDescVec.data(),
                                                              static_cast<std::uint32_t>(metalnessSrvDescVec.size()));

    mRoughnessTextureRenderTargetViewsBegin =
        CbvSrvUavDescriptorManager::CreateShaderResourceViews(roughnessResVec.data(),
                                                              roughnessSrvDescVec.data(),
                                                              static_cast<std::uint32_t>(roughnessSrvDescVec.size()));

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
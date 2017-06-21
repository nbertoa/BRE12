#include "TextureMappingCommandListRecorder.h"

#include <DirectXMath.h>

#include <CommandListExecutor\CommandListExecutor.h>
#include <DescriptorManager\CbvSrvUavDescriptorManager.h>
#include <DirectXManager\DirectXManager.h>
#include <MathUtils/MathUtils.h>
#include <PSOManager/PSOManager.h>
#include <ResourceManager/UploadBufferManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <ShaderManager\ShaderManager.h>
#include <ShaderUtils\CBuffers.h>
#include <ShaderUtils\MaterialProperties.h>
#include <Utils/DebugUtils.h>

namespace BRE {
// Root Signature:
// "DescriptorTable(CBV(b0), visibility = SHADER_VISIBILITY_VERTEX), " \ 0 -> Object CBuffers
// "CBV(b1, visibility = SHADER_VISIBILITY_VERTEX), " \ 1 -> Frame CBuffer
// "DescriptorTable(CBV(b0), visibility = SHADER_VISIBILITY_PIXEL), " \ 2 -> Material CBuffers
// "CBV(b1, visibility = SHADER_VISIBILITY_PIXEL), " \ 3 -> Frame CBuffer
// "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL), " \ 4 -> Diffuse Texture
// "DescriptorTable(SRV(t1), visibility = SHADER_VISIBILITY_PIXEL), " \ 5 -> Metalness Texture
// "DescriptorTable(SRV(t2), visibility = SHADER_VISIBILITY_PIXEL), " \ 6 -> Roughness Texture

namespace {
ID3D12PipelineState* sPSO{ nullptr };
ID3D12RootSignature* sRootSignature{ nullptr };
}

void
TextureMappingCommandListRecorder::InitSharedPSOAndRootSignature(const DXGI_FORMAT* geometryBufferFormats,
                                                                 const std::uint32_t geometryBufferCount) noexcept
{
    BRE_ASSERT(geometryBufferFormats != nullptr);
    BRE_ASSERT(geometryBufferCount > 0U);
    BRE_ASSERT(sPSO == nullptr);
    BRE_ASSERT(sRootSignature == nullptr);

    // Build pso and root signature
    PSOManager::PSOCreationData psoData{};
    psoData.mInputLayoutDescriptors = D3DFactory::GetPositionNormalTangentTexCoordInputLayout();

    psoData.mPixelShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("GeometryPass/Shaders/TextureMapping/PS.cso");
    psoData.mVertexShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("GeometryPass/Shaders/TextureMapping/VS.cso");

    ID3DBlob* rootSignatureBlob = &ShaderManager::LoadShaderFileAndGetBlob("GeometryPass/Shaders/TextureMapping/RS.cso");
    psoData.mRootSignature = &RootSignatureManager::CreateRootSignatureFromBlob(*rootSignatureBlob);
    sRootSignature = psoData.mRootSignature;

    psoData.mNumRenderTargets = geometryBufferCount;
    memcpy(psoData.mRenderTargetFormats, geometryBufferFormats, sizeof(DXGI_FORMAT) * psoData.mNumRenderTargets);
    sPSO = &PSOManager::CreateGraphicsPSO(psoData);

    BRE_ASSERT(sPSO != nullptr);
    BRE_ASSERT(sRootSignature != nullptr);
}

void
TextureMappingCommandListRecorder::Init(const std::vector<GeometryData>& geometryDataVector,
                                        const std::vector<MaterialProperties>& materialProperties,
                                        const std::vector<ID3D12Resource*>& diffuseTextures,
                                        const std::vector<ID3D12Resource*>& metalnessTextures,
                                        const std::vector<ID3D12Resource*>& roughnessTextures) noexcept
{
    BRE_ASSERT(geometryDataVector.empty() == false);
    BRE_ASSERT(materialProperties.empty() == false);
    BRE_ASSERT(materialProperties.size() == diffuseTextures.size());
    BRE_ASSERT(diffuseTextures.size() == metalnessTextures.size());
    BRE_ASSERT(metalnessTextures.size() == roughnessTextures.size());
    BRE_ASSERT(IsDataValid() == false);

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
                         diffuseTextures,
                         metalnessTextures,
                         roughnessTextures);

    BRE_ASSERT(IsDataValid());
}

std::uint32_t
TextureMappingCommandListRecorder::RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept
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
    D3D12_GPU_DESCRIPTOR_HANDLE baseColorTextureRenderTargetView(mBaseColorTextureRenderTargetViewsBegin);
    D3D12_GPU_DESCRIPTOR_HANDLE metalnessTextureRenderTargetView(mMetalnessTextureRenderTargetViewsBegin);
    D3D12_GPU_DESCRIPTOR_HANDLE roughnessTextureRenderTargetView(mRoughnessTextureRenderTargetViewsBegin);

    commandList.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set frame constants root parameters
    D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(uploadFrameCBuffer.GetResource().GetGPUVirtualAddress());
    commandList.SetGraphicsRootConstantBufferView(1U, frameCBufferGpuVAddress);
    commandList.SetGraphicsRootConstantBufferView(3U, frameCBufferGpuVAddress);

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

            commandList.SetGraphicsRootDescriptorTable(2U, materialCBufferView);
            materialCBufferView.ptr += descHandleIncSize;

            commandList.SetGraphicsRootDescriptorTable(4U, baseColorTextureRenderTargetView);
            baseColorTextureRenderTargetView.ptr += descHandleIncSize;

            commandList.SetGraphicsRootDescriptorTable(5U, metalnessTextureRenderTargetView);
            metalnessTextureRenderTargetView.ptr += descHandleIncSize;

            commandList.SetGraphicsRootDescriptorTable(6U, roughnessTextureRenderTargetView);
            roughnessTextureRenderTargetView.ptr += descHandleIncSize;

            commandList.DrawIndexedInstanced(geomData.mIndexBufferData.mElementCount, 1U, 0U, 0U, 0U);
        }
    }

    commandList.Close();
    CommandListExecutor::Get().PushCommandList(commandList);

    return 1U;
}

bool
TextureMappingCommandListRecorder::IsDataValid() const noexcept
{
    const std::size_t geometryDataCount{ mGeometryDataVec.size() };
    for (std::size_t i = 0UL; i < geometryDataCount; ++i) {
        const std::size_t numMatrices{ mGeometryDataVec[i].mWorldMatrices.size() };
        if (numMatrices == 0UL) {
            return false;
        }
    }

    const bool result =
        GeometryCommandListRecorder::IsDataValid() &&
        mBaseColorTextureRenderTargetViewsBegin.ptr != 0UL &&
        mMetalnessTextureRenderTargetViewsBegin.ptr != 0UL &&
        mRoughnessTextureRenderTargetViewsBegin.ptr != 0UL;

    return result;
}

void
TextureMappingCommandListRecorder::InitCBuffersAndViews(const std::vector<MaterialProperties>& materialProperties,
                                                        const std::vector<ID3D12Resource*>& diffuseTextures,
                                                        const std::vector<ID3D12Resource*>& metalnessTextures,
                                                        const std::vector<ID3D12Resource*>& roughnessTextures) noexcept
{
    BRE_ASSERT(materialProperties.empty() == false);
    BRE_ASSERT(materialProperties.size() == diffuseTextures.size());
    BRE_ASSERT(diffuseTextures.size() == metalnessTextures.size());
    BRE_ASSERT(metalnessTextures.size() == roughnessTextures.size());
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
            mObjectUploadCBuffers->CopyData(k + j, 
                                            &objCBuffer, 
                                            sizeof(objCBuffer));
        }

        k += worldMatsCount;
    }

    // Create material properties cbuffer		
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

    std::vector<ID3D12Resource*> resVec;
    resVec.reserve(numResources);
    std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> srvDescVec;
    srvDescVec.reserve(numResources);

    std::vector<ID3D12Resource*> metalnessResVec;
    metalnessResVec.reserve(numResources);
    std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> metalnessSrvDescVec;
    metalnessSrvDescVec.reserve(numResources);

    std::vector<ID3D12Resource*> roughnessResVec;
    roughnessResVec.reserve(numResources);
    std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> roughnessSrvDescVec;
    roughnessSrvDescVec.reserve(numResources);
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

        // Texture descriptor
        resVec.push_back(diffuseTextures[i]);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        srvDesc.Format = resVec.back()->GetDesc().Format;
        srvDesc.Texture2D.MipLevels = resVec.back()->GetDesc().MipLevels;
        srvDescVec.push_back(srvDesc);

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

        mMaterialUploadCBuffers->CopyData(static_cast<std::uint32_t>(i), &materialProperties[i], sizeof(MaterialProperties));
    }

    mObjectCBufferViewsBegin =
        CbvSrvUavDescriptorManager::CreateConstantBufferViews(objectCbufferViewDescVec.data(),
                                                              static_cast<std::uint32_t>(objectCbufferViewDescVec.size()));

    mMaterialCBufferViewsBegin =
        CbvSrvUavDescriptorManager::CreateConstantBufferViews(materialCbufferViewDescVec.data(),
                                                              static_cast<std::uint32_t>(materialCbufferViewDescVec.size()));

    mBaseColorTextureRenderTargetViewsBegin =
        CbvSrvUavDescriptorManager::CreateShaderResourceViews(resVec.data(),
                                                              srvDescVec.data(),
                                                              static_cast<std::uint32_t>(srvDescVec.size()));

    mMetalnessTextureRenderTargetViewsBegin =
        CbvSrvUavDescriptorManager::CreateShaderResourceViews(metalnessResVec.data(),
                                                              metalnessSrvDescVec.data(),
                                                              static_cast<std::uint32_t>(metalnessSrvDescVec.size()));

    mRoughnessTextureRenderTargetViewsBegin =
        CbvSrvUavDescriptorManager::CreateShaderResourceViews(roughnessResVec.data(),
                                                              roughnessSrvDescVec.data(),
                                                              static_cast<std::uint32_t>(roughnessSrvDescVec.size()));

}
}
#include "AmbientOcclusionCommandListRecorder.h"

#include <ApplicationSettings\ApplicationSettings.h>
#include <CommandListExecutor\CommandListExecutor.h>
#include <DescriptorManager\CbvSrvUavDescriptorManager.h>
#include <DXUtils\D3DFactory.h>
#include <DXUtils\d3dx12.h>
#include <EnvironmentLightPass\EnvironmentLightSettings.h>
#include <EnvironmentLightPass\Shaders\AmbientOcclusionCBuffer.h>
#include <MathUtils/MathUtils.h>
#include <PSOManager/PSOManager.h>
#include <ResourceManager/ResourceManager.h>
#include <ResourceManager/UploadBufferManager.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <RootSignatureManager\RootSignatureManager.h>
#include <ShaderManager\ShaderManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils/DebugUtils.h>

using namespace DirectX;

namespace BRE {
// Root Signature:
// "CBV(b0, visibility = SHADER_VISIBILITY_VERTEX), " \ 0 -> Frame CBuffer
// "CBV(b0, visibility = SHADER_VISIBILITY_PIXEL), " \ 1 -> Frame CBuffer
// "CBV(b1, visibility = SHADER_VISIBILITY_PIXEL), " \ 2 -> Ambient Occlusion CBuffer
// "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL)" 3 -> normal_roughness
// "DescriptorTable(SRV(t1), SRV(t2), visibility = SHADER_VISIBILITY_PIXEL)" 4 -> sample kernel + kernel noise
// "DescriptorTable(SRV(t3), visibility = SHADER_VISIBILITY_PIXEL)" 5 -> depth buffer

namespace {
ID3D12PipelineState* sPSO{ nullptr };
ID3D12RootSignature* sRootSignature{ nullptr };

///
/// @brief Generates sample kernel
///
/// Sample kernel for ambient occlusion. The requirements are that:
/// - Sample positions fall within the unit hemisphere oriented
///   toward positive z axis.
/// - Sample positions are more densely clustered towards the origin.
///   This effectively attenuates the occlusion contribution
///   according to distance from the sample kernel centre (samples closer
///   to a point occlude it more than samples further away).
///
/// @param sampleKernelSize Size of the sample kernel to generate
/// @param sampleKernel Output sample kernel list
///
void
GenerateSampleKernel(const std::uint32_t sampleKernelSize,
                     std::vector<XMFLOAT4>& sampleKernel)
{
    BRE_ASSERT(sampleKernelSize > 0U);

    sampleKernel.reserve(sampleKernelSize);
    const float sampleKernelSizeFloat = static_cast<float>(sampleKernelSize);
    XMVECTOR vec;
    for (std::uint32_t i = 0U; i < sampleKernelSize; ++i) {
        const float x = MathUtils::RandomFloatInInterval(-1.0f, 1.0f);
        const float y = MathUtils::RandomFloatInInterval(-1.0f, 1.0f);
        const float z = MathUtils::RandomFloatInInterval(0.0f, 1.0f);
        sampleKernel.push_back(XMFLOAT4(x, y, z, 0.0f));
        XMFLOAT4& currentSample = sampleKernel.back();
        vec = XMLoadFloat4(&currentSample);
        vec = XMVector4Normalize(vec);
        XMStoreFloat4(&currentSample, vec);

        // Accelerating interpolation function to falloff 
        // from the distance from the origin.
        float scale = i / sampleKernelSizeFloat;
        scale = MathUtils::Lerp(0.1f, 1.0f, scale * scale);
        vec = XMVectorScale(vec, scale);
        XMStoreFloat4(&currentSample, vec);
    }
}

///
/// @brief Generates noise vector
///
/// Generate a set of random values used to rotate the sample kernel,
/// which will effectively increase the sample count and minimize 
/// the 'banding' artifacts.
///
/// @param numSamples Number of samples to generate
/// @param noiseVector Output noise vector
///
void
GenerateNoise(const std::uint32_t numSamples,
              std::vector<XMFLOAT4>& noiseVector)
{
    BRE_ASSERT(numSamples > 0U);

    noiseVector.reserve(numSamples);
    XMVECTOR vec;
    for (std::uint32_t i = 0U; i < numSamples; ++i) {
        const float x = MathUtils::RandomFloatInInterval(-1.0f, 1.0f);
        const float y = MathUtils::RandomFloatInInterval(-1.0f, 1.0f);
        // The z component must zero. Since our kernel is oriented along the z-axis, 
        // we want the random rotation to occur around that axis.
        const float z = 0.0f;
        noiseVector.push_back(XMFLOAT4(x, y, z, 0.0f));
        XMFLOAT4& currentSample = noiseVector.back();
        vec = XMLoadFloat4(&currentSample);
        vec = XMVector4Normalize(vec);
        XMStoreFloat4(&currentSample, vec);

        // Map from [-1.0f, 1.0f] to [0.0f, 1.0f] because
        // this is going to be stored in a texture
        currentSample.x = currentSample.x * 0.5f + 0.5f;
        currentSample.y = currentSample.y * 0.5f + 0.5f;
        currentSample.z = currentSample.z * 0.5f + 0.5f;
    }
}
}

void
AmbientOcclusionCommandListRecorder::InitSharedPSOAndRootSignature() noexcept
{
    BRE_ASSERT(sPSO == nullptr);
    BRE_ASSERT(sRootSignature == nullptr);

    PSOManager::PSOCreationData psoData{};
    psoData.mBlendDescriptor = D3DFactory::GetAlwaysBlendDesc();
    psoData.mDepthStencilDescriptor = D3DFactory::GetDisabledDepthStencilDesc();

    psoData.mPixelShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("EnvironmentLightPass/Shaders/AmbientOcclusion/PS.cso");
    psoData.mVertexShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("EnvironmentLightPass/Shaders/AmbientOcclusion/VS.cso");

    ID3DBlob* rootSignatureBlob = &ShaderManager::LoadShaderFileAndGetBlob("EnvironmentLightPass/Shaders/AmbientOcclusion/RS.cso");
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
AmbientOcclusionCommandListRecorder::Init(const D3D12_CPU_DESCRIPTOR_HANDLE& ambientAccessibilityBufferRenderTargetView,
                                          const D3D12_GPU_DESCRIPTOR_HANDLE& normalRoughnessBufferShaderResourceView,
                                          const D3D12_GPU_DESCRIPTOR_HANDLE& depthBufferShaderResourceView) noexcept
{
    BRE_ASSERT(IsDataValid() == false);
    
    mAmbientAccessibilityBufferRenderTargetView = ambientAccessibilityBufferRenderTargetView;
    mNormalRoughnessBufferShaderResourceView = normalRoughnessBufferShaderResourceView;
    mDepthBufferShaderResourceView = depthBufferShaderResourceView;

    const std::uint32_t sampleKernelSize = 
        static_cast<std::uint32_t>(EnvironmentLightSettings::sSampleKernelSize);
    const std::uint32_t noiseTextureDimension = 
        static_cast<std::uint32_t>(EnvironmentLightSettings::sNoiseTextureDimension);

    std::vector<XMFLOAT4> sampleKernel;
    GenerateSampleKernel(sampleKernelSize, sampleKernel);
    std::vector<XMFLOAT4> noises;
    GenerateNoise(noiseTextureDimension * noiseTextureDimension, noises);

    CreateSampleKernelBuffer(sampleKernel);
    ID3D12Resource* noiseTexture = CreateAndGetNoiseTexture(noises);
    BRE_ASSERT(noiseTexture != nullptr);
    InitShaderResourceViews(*noiseTexture,
                            sampleKernelSize);

    InitAmbientOcclusionCBuffer();

    BRE_ASSERT(IsDataValid());
}

std::uint32_t
AmbientOcclusionCommandListRecorder::RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept
{
    BRE_ASSERT(IsDataValid());
    BRE_ASSERT(sPSO != nullptr);
    BRE_ASSERT(sRootSignature != nullptr);

    ID3D12GraphicsCommandList& commandList = mCommandListPerFrame.ResetCommandListWithNextCommandAllocator(sPSO);

    // Update frame constants
    UploadBuffer& uploadFrameCBuffer(mFrameUploadCBufferPerFrame.GetNextFrameCBuffer());
    uploadFrameCBuffer.CopyData(0U, &frameCBuffer, sizeof(frameCBuffer));

    commandList.RSSetViewports(1U, &ApplicationSettings::sScreenViewport);
    commandList.RSSetScissorRects(1U, &ApplicationSettings::sScissorRect);
    commandList.OMSetRenderTargets(1U, 
                                   &mAmbientAccessibilityBufferRenderTargetView, 
                                   false, 
                                   nullptr);

    ID3D12DescriptorHeap* heaps[] = { &CbvSrvUavDescriptorManager::GetDescriptorHeap() };
    commandList.SetDescriptorHeaps(_countof(heaps), heaps);

    commandList.SetGraphicsRootSignature(sRootSignature);
    const D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(
        uploadFrameCBuffer.GetResource().GetGPUVirtualAddress());
    const D3D12_GPU_VIRTUAL_ADDRESS ambientOcclusionCBufferGpuVAddress(
        mAmbientOcclusionUploadCBuffer->GetResource().GetGPUVirtualAddress());
    commandList.SetGraphicsRootConstantBufferView(0U, frameCBufferGpuVAddress);
    commandList.SetGraphicsRootConstantBufferView(1U, frameCBufferGpuVAddress);
    commandList.SetGraphicsRootConstantBufferView(2U, ambientOcclusionCBufferGpuVAddress);
    commandList.SetGraphicsRootDescriptorTable(3U, mNormalRoughnessBufferShaderResourceView);
    commandList.SetGraphicsRootDescriptorTable(4U, mPixelShaderResourceViewsBegin);
    commandList.SetGraphicsRootDescriptorTable(5U, mDepthBufferShaderResourceView);

    commandList.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList.DrawInstanced(6U, 1U, 0U, 0U);

    commandList.Close();
    CommandListExecutor::Get().PushCommandList(commandList);

    return 1U;
}

bool
AmbientOcclusionCommandListRecorder::IsDataValid() const noexcept
{
    const bool result =
        mSampleKernelUploadBuffer != nullptr &&
        mAmbientAccessibilityBufferRenderTargetView.ptr != 0UL &&

        mNormalRoughnessBufferShaderResourceView.ptr != 0UL &&
        mDepthBufferShaderResourceView.ptr != 0UL &&
        mPixelShaderResourceViewsBegin.ptr != 0UL &&
        mAmbientOcclusionUploadCBuffer != nullptr;

    return result;
}

void
AmbientOcclusionCommandListRecorder::CreateSampleKernelBuffer(const std::vector<XMFLOAT4>& sampleKernel) noexcept
{
    BRE_ASSERT(mSampleKernelUploadBuffer == nullptr);
    BRE_ASSERT(sampleKernel.empty() == false);

    const std::uint32_t sampleKernelSize = static_cast<std::uint32_t>(sampleKernel.size());

    const std::size_t sampleKernelBufferElemSize{ sizeof(XMFLOAT4) };
    mSampleKernelUploadBuffer = &UploadBufferManager::CreateUploadBuffer(sampleKernelBufferElemSize,
                                                                         sampleKernelSize);
    const std::uint8_t* sampleKernelPtr = reinterpret_cast<const std::uint8_t*>(sampleKernel.data());
    for (std::uint32_t i = 0UL; i < sampleKernelSize; ++i) {
        mSampleKernelUploadBuffer->CopyData(i,
                                            sampleKernelPtr + sampleKernelBufferElemSize * i,
                                            sampleKernelBufferElemSize);
    }
}

ID3D12Resource*
AmbientOcclusionCommandListRecorder::CreateAndGetNoiseTexture(const std::vector<XMFLOAT4>& noiseVector) noexcept
{
    BRE_ASSERT(noiseVector.empty() == false);

    const std::uint32_t noiseVectorCount = static_cast<std::uint32_t>(noiseVector.size());

    // Kernel noise resource and fill it
    D3D12_RESOURCE_DESC resourceDescriptor = D3DFactory::GetResourceDescriptor(noiseVectorCount,
                                                                               noiseVectorCount,
                                                                               DXGI_FORMAT_R16G16B16A16_UNORM,
                                                                               D3D12_RESOURCE_FLAG_NONE);

    // Create noise texture and fill it.
    ID3D12Resource* noiseTexture{ nullptr };
    D3D12_HEAP_PROPERTIES heapProperties = D3DFactory::GetHeapProperties();
    noiseTexture = &ResourceManager::CreateCommittedResource(heapProperties,
                                                             D3D12_HEAP_FLAG_NONE,
                                                             resourceDescriptor,
                                                             D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                                             nullptr,
                                                             L"Noise Buffer",
                                                             ResourceManager::ResourceStateTrackingType::FULL_TRACKING);

    // In order to copy CPU memory data into our default buffer, we need to create
    // an intermediate upload heap. 
    const std::uint32_t num2DSubresources = resourceDescriptor.DepthOrArraySize * resourceDescriptor.MipLevels;
    const std::size_t uploadBufferSize = GetRequiredIntermediateSize(noiseTexture, 0, num2DSubresources);
    ID3D12Resource* noiseTextureUploadBuffer{ nullptr }; 

    heapProperties = D3DFactory::GetHeapProperties(D3D12_HEAP_TYPE_UPLOAD,
                                                   D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                                                   D3D12_MEMORY_POOL_UNKNOWN,
                                                   1U,
                                                   0U);

    resourceDescriptor = D3DFactory::GetResourceDescriptor(uploadBufferSize,
                                                           1,
                                                           DXGI_FORMAT_UNKNOWN,
                                                           D3D12_RESOURCE_FLAG_NONE,
                                                           D3D12_RESOURCE_DIMENSION_BUFFER,
                                                           D3D12_TEXTURE_LAYOUT_ROW_MAJOR);

    noiseTextureUploadBuffer = &ResourceManager::CreateCommittedResource(heapProperties,
                                                                         D3D12_HEAP_FLAG_NONE,
                                                                         resourceDescriptor,
                                                                         D3D12_RESOURCE_STATE_GENERIC_READ,
                                                                         nullptr,
                                                                         nullptr,
                                                                         ResourceManager::ResourceStateTrackingType::NO_TRACKING);

    return noiseTexture;
}

void
AmbientOcclusionCommandListRecorder::InitShaderResourceViews(ID3D12Resource& noiseTexture,
                                                             const std::uint32_t sampleKernelSize) noexcept
{
    BRE_ASSERT(mSampleKernelUploadBuffer != nullptr);
    BRE_ASSERT(sampleKernelSize != 0U);

    ID3D12Resource* resources[] =
    {
        &mSampleKernelUploadBuffer->GetResource(),
        &noiseTexture,
    };

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptors[_countof(resources)]{};

    // Fill sample kernel buffer descriptor
    srvDescriptors[0].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptors[0].Format = mSampleKernelUploadBuffer->GetResource().GetDesc().Format;
    srvDescriptors[0].ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDescriptors[0].Buffer.FirstElement = 0UL;
    srvDescriptors[0].Buffer.NumElements = sampleKernelSize;
    srvDescriptors[0].Buffer.StructureByteStride = sizeof(XMFLOAT4);

    // Fill kernel noise texture descriptor
    srvDescriptors[1].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptors[1].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescriptors[1].Texture2D.MostDetailedMip = 0;
    srvDescriptors[1].Texture2D.ResourceMinLODClamp = 0.0f;
    srvDescriptors[1].Format = noiseTexture.GetDesc().Format;
    srvDescriptors[1].Texture2D.MipLevels = noiseTexture.GetDesc().MipLevels;

    BRE_ASSERT(_countof(resources) == _countof(srvDescriptors));

    mPixelShaderResourceViewsBegin = CbvSrvUavDescriptorManager::CreateShaderResourceViews(resources,
                                                                                          srvDescriptors,
                                                                                          _countof(srvDescriptors));
}

void
AmbientOcclusionCommandListRecorder::InitAmbientOcclusionCBuffer() noexcept
{
    const std::size_t ambientOcclusionUploadCBufferElemSize =
        UploadBuffer::GetRoundedConstantBufferSizeInBytes(sizeof(AmbientOcclusionCBuffer));

    mAmbientOcclusionUploadCBuffer = &UploadBufferManager::CreateUploadBuffer(ambientOcclusionUploadCBufferElemSize,
                                                                              1U);
    AmbientOcclusionCBuffer ambientOcclusionCBuffer(static_cast<float>(ApplicationSettings::sWindowWidth),
                                                    static_cast<float>(ApplicationSettings::sWindowHeight),
                                                    EnvironmentLightSettings::sSampleKernelSize,
                                                    EnvironmentLightSettings::sNoiseTextureDimension,
                                                    EnvironmentLightSettings::sOcclusionRadius,
                                                    EnvironmentLightSettings::sSsaoPower);

    mAmbientOcclusionUploadCBuffer->CopyData(0U, &ambientOcclusionCBuffer, sizeof(AmbientOcclusionCBuffer));
}

}
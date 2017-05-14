#include "AmbientOcclusionCommandListRecorder.h"

#include <CommandListExecutor\CommandListExecutor.h>
#include <DescriptorManager\CbvSrvUavDescriptorManager.h>
#include <DXUtils\d3dx12.h>
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
// "DescriptorTable(SRV(t0), SRV(t1), SRV(t2), SRV(t3), visibility = SHADER_VISIBILITY_PIXEL)" 2 -> normal_smoothness + depth + sample kernel + kernel noise

namespace {
ID3D12PipelineState* sPSO{ nullptr };
ID3D12RootSignature* sRootSignature{ nullptr };

// Sample kernel for ambient occlusion. The requirements are that:
// - Sample positions fall within the unit hemisphere oriented
//   toward positive z axis.
// - Sample positions are more densely clustered towards the origin.
//   This effectively attenuates the occlusion contribution
//   according to distance from the sample kernel centre (samples closer
//   to a point occlude it more than samples further away).
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
        const float x = MathUtils::RandomFloatInInverval(-1.0f, 1.0f);
        const float y = MathUtils::RandomFloatInInverval(-1.0f, 1.0f);
        const float z = MathUtils::RandomFloatInInverval(0.0f, 1.0f);
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
        const float x = MathUtils::RandomFloatInInverval(-1.0f, 1.0f);
        const float y = MathUtils::RandomFloatInInverval(-1.0f, 1.0f);
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
AmbientOcclusionCommandListRecorder::Init(ID3D12Resource& normalSmoothnessBuffer,
                                      ID3D12Resource& depthBuffer,
                                      const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept
{
    BRE_ASSERT(ValidateData() == false);

    mRenderTargetView = renderTargetView;

    const std::uint32_t sampleKernelSize = 64U;
    const std::uint32_t noiseTextureDimension = 4U;

    std::vector<XMFLOAT4> sampleKernel;
    GenerateSampleKernel(sampleKernelSize, sampleKernel);
    std::vector<XMFLOAT4> noises;
    GenerateNoise(noiseTextureDimension * noiseTextureDimension, noises);

    CreateSampleKernelBuffer(sampleKernel);
    ID3D12Resource* noiseTexture = CreateAndGetNoiseTexture(noises);
    BRE_ASSERT(noiseTexture != nullptr);
    InitShaderResourceViews(normalSmoothnessBuffer,
                            depthBuffer,
                            *noiseTexture,
                            sampleKernelSize);

    BRE_ASSERT(ValidateData());
}

void
AmbientOcclusionCommandListRecorder::RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept
{
    BRE_ASSERT(ValidateData());
    BRE_ASSERT(sPSO != nullptr);
    BRE_ASSERT(sRootSignature != nullptr);

    ID3D12GraphicsCommandList& commandList = mCommandListPerFrame.ResetCommandListWithNextCommandAllocator(sPSO);

    // Update frame constants
    UploadBuffer& uploadFrameCBuffer(mFrameUploadCBufferPerFrame.GetNextFrameCBuffer());
    uploadFrameCBuffer.CopyData(0U, &frameCBuffer, sizeof(frameCBuffer));

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
AmbientOcclusionCommandListRecorder::ValidateData() const noexcept
{
    const bool result =
        mSampleKernelUploadBuffer != nullptr &&
        mRenderTargetView.ptr != 0UL &&
        mStartPixelShaderResourceView.ptr != 0UL;

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
    D3D12_RESOURCE_DESC resourceDescriptor = {};
    resourceDescriptor.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDescriptor.Alignment = 0U;
    resourceDescriptor.Width = noiseVectorCount;
    resourceDescriptor.Height = noiseVectorCount;
    resourceDescriptor.DepthOrArraySize = 1U;
    resourceDescriptor.MipLevels = 1U;
    resourceDescriptor.SampleDesc.Count = 1U;
    resourceDescriptor.SampleDesc.Quality = 0U;
    resourceDescriptor.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDescriptor.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
    resourceDescriptor.Flags = D3D12_RESOURCE_FLAG_NONE;

    // Create noise texture and fill it.
    ID3D12Resource* noiseTexture{ nullptr };
    CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
    noiseTexture = &ResourceManager::CreateCommittedResource(heapProps,
                                                             D3D12_HEAP_FLAG_NONE,
                                                             resourceDescriptor,
                                                             D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                                             nullptr,
                                                             L"Noise Buffer");

    // In order to copy CPU memory data into our default buffer, we need to create
    // an intermediate upload heap. 
    const std::uint32_t num2DSubresources = resourceDescriptor.DepthOrArraySize * resourceDescriptor.MipLevels;
    const std::size_t uploadBufferSize = GetRequiredIntermediateSize(noiseTexture, 0, num2DSubresources);
    ID3D12Resource* noiseTextureUploadBuffer{ nullptr };
    noiseTextureUploadBuffer = &ResourceManager::CreateCommittedResource(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                                                         D3D12_HEAP_FLAG_NONE,
                                                                         CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
                                                                         D3D12_RESOURCE_STATE_GENERIC_READ,
                                                                         nullptr,
                                                                         nullptr);

    return noiseTexture;
}

void
AmbientOcclusionCommandListRecorder::InitShaderResourceViews(ID3D12Resource& normalSmoothnessBuffer,
                                                         ID3D12Resource& depthBuffer,
                                                         ID3D12Resource& noiseTexture,
                                                         const std::uint32_t sampleKernelSize) noexcept
{
    BRE_ASSERT(mSampleKernelUploadBuffer != nullptr);
    BRE_ASSERT(sampleKernelSize != 0U);

    ID3D12Resource* resources[] =
    {
        &normalSmoothnessBuffer,
        &depthBuffer,
        mSampleKernelUploadBuffer->GetResource(),
        &noiseTexture,
    };

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptors[4U]{};

    // Fill normal_smoothness buffer texture descriptor
    srvDescriptors[0].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptors[0].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescriptors[0].Texture2D.MostDetailedMip = 0;
    srvDescriptors[0].Texture2D.ResourceMinLODClamp = 0.0f;
    srvDescriptors[0].Format = normalSmoothnessBuffer.GetDesc().Format;
    srvDescriptors[0].Texture2D.MipLevels = normalSmoothnessBuffer.GetDesc().MipLevels;

    // Fill depth buffer descriptor
    srvDescriptors[1].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptors[1].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescriptors[1].Texture2D.MostDetailedMip = 0;
    srvDescriptors[1].Texture2D.ResourceMinLODClamp = 0.0f;
    srvDescriptors[1].Format = SettingsManager::sDepthStencilSRVFormat;
    srvDescriptors[1].Texture2D.MipLevels = depthBuffer.GetDesc().MipLevels;

    // Fill sample kernel buffer descriptor
    srvDescriptors[2].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptors[2].Format = mSampleKernelUploadBuffer->GetResource()->GetDesc().Format;
    srvDescriptors[2].ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDescriptors[2].Buffer.FirstElement = 0UL;
    srvDescriptors[2].Buffer.NumElements = sampleKernelSize;
    srvDescriptors[2].Buffer.StructureByteStride = sizeof(XMFLOAT4);

    // Fill kernel noise texture descriptor
    srvDescriptors[3].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptors[3].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescriptors[3].Texture2D.MostDetailedMip = 0;
    srvDescriptors[3].Texture2D.ResourceMinLODClamp = 0.0f;
    srvDescriptors[3].Format = noiseTexture.GetDesc().Format;
    srvDescriptors[3].Texture2D.MipLevels = noiseTexture.GetDesc().MipLevels;

    BRE_ASSERT(_countof(resources) == _countof(srvDescriptors));

    // Create SRVs
    mStartPixelShaderResourceView = CbvSrvUavDescriptorManager::CreateShaderResourceViews(resources,
                                                                                          srvDescriptors,
                                                                                          _countof(srvDescriptors));
}
}


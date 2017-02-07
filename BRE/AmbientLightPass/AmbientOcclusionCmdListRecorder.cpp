#include "AmbientOcclusionCmdListRecorder.h"

#include <DirectXMath.h>

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
	void GenerateSampleKernel(const std::uint32_t sampleKernelSize, std::vector<XMFLOAT4>& sampleKernel) {
		ASSERT(sampleKernelSize > 0U);

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

	// Generate a set of random values used to rotate the sample kernel,
	// which will effectively increase the sample count and minimize 
	// the 'banding' artifacts.
	void GenerateNoise(const std::uint32_t numSamples, std::vector<XMFLOAT4>& noiseVectors) {
		ASSERT(numSamples > 0U);

		noiseVectors.reserve(numSamples);
		XMVECTOR vec;
		for (std::uint32_t i = 0U; i < numSamples; ++i) {
			const float x = MathUtils::RandomFloatInInverval(-1.0f, 1.0f);
			const float y = MathUtils::RandomFloatInInverval(-1.0f, 1.0f);
			// The z component must zero. Since our kernel is oriented along the z-axis, 
			// we want the random rotation to occur around that axis.
			const float z = 0.0f;
			noiseVectors.push_back(XMFLOAT4(x, y, z, 0.0f));
			XMFLOAT4& currentSample = noiseVectors.back();
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

void AmbientOcclusionCmdListRecorder::InitPSO() noexcept {
	ASSERT(sPSO == nullptr);
	ASSERT(sRootSignature == nullptr);

	// Build pso and root signature
	PSOManager::PSOCreationData psoData{};
	const std::size_t renderTargetCount{ _countof(psoData.mRenderTargetFormats) };
	psoData.mBlendDescriptor = D3DFactory::GetAlwaysBlendDesc();
	psoData.mDepthStencilDescriptor = D3DFactory::GetDisabledDepthStencilDesc();

	psoData.mPixelShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("AmbientLightPass/Shaders/AmbientOcclusion/PS.cso");
	psoData.mVertexShaderBytecode = ShaderManager::LoadShaderFileAndGetBytecode("AmbientLightPass/Shaders/AmbientOcclusion/VS.cso");

	ID3DBlob* rootSignatureBlob = &ShaderManager::LoadShaderFileAndGetBlob("AmbientLightPass/Shaders/AmbientOcclusion/RS.cso");
	psoData.mRootSignature = &RootSignatureManager::CreateRootSignatureFromBlob(*rootSignatureBlob);
	sRootSignature = psoData.mRootSignature;

	psoData.mNumRenderTargets = 1U;
	psoData.mRenderTargetFormats[0U] = DXGI_FORMAT_R16_UNORM;
	for (std::size_t i = psoData.mNumRenderTargets; i < renderTargetCount; ++i) {
		psoData.mRenderTargetFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	psoData.mPrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	sPSO = &PSOManager::CreateGraphicsPSO(psoData);

	ASSERT(sPSO != nullptr);
	ASSERT(sRootSignature != nullptr);
}

void AmbientOcclusionCmdListRecorder::Init(
	ID3D12Resource& normalSmoothnessBuffer,	
	const D3D12_CPU_DESCRIPTOR_HANDLE& ambientAccessBufferCpuDesc,
	ID3D12Resource& depthBuffer) noexcept
{
	ASSERT(ValidateData() == false);

	mAmbientAccessibilityBufferCpuDesc = ambientAccessBufferCpuDesc;

	mSampleKernelSize = 128U;
	mNoiseTextureDimension = 4U;
	std::vector<XMFLOAT4> sampleKernel;
	GenerateSampleKernel(mSampleKernelSize, sampleKernel);
	std::vector<XMFLOAT4> noises;
	GenerateNoise(mNoiseTextureDimension * mNoiseTextureDimension, noises);

	InitConstantBuffers();
	CreateSampleKernelBuffer(sampleKernel.data());
	ID3D12Resource* noiseTexture = CreateAndGetNoiseTexture(noises.data());
	ASSERT(noiseTexture != nullptr);
	InitShaderResourceViews(
		normalSmoothnessBuffer,
		depthBuffer,
		*noiseTexture);

	ASSERT(ValidateData());
}

void AmbientOcclusionCmdListRecorder::RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(ValidateData());
	ASSERT(sPSO != nullptr);
	ASSERT(sRootSignature != nullptr);

	static std::uint32_t currentFrameIndex = 0U;

	ID3D12GraphicsCommandList& commandList = mCommandListPerFrame.ResetWithNextCommandAllocator(sPSO);

	// Update frame constants
	UploadBuffer& uploadFrameCBuffer(*mFrameCBuffer[currentFrameIndex]);
	uploadFrameCBuffer.CopyData(0U, &frameCBuffer, sizeof(frameCBuffer));

	commandList.RSSetViewports(1U, &SettingsManager::sScreenViewport);
	commandList.RSSetScissorRects(1U, &SettingsManager::sScissorRect);
	commandList.OMSetRenderTargets(1U, &mAmbientAccessibilityBufferCpuDesc, false, nullptr);

	ID3D12DescriptorHeap* heaps[] = { &CbvSrvUavDescriptorManager::GetDescriptorHeap() };
	commandList.SetDescriptorHeaps(_countof(heaps), heaps);
	commandList.SetGraphicsRootSignature(sRootSignature);	

	// Set root parameters
	const D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(uploadFrameCBuffer.GetResource()->GetGPUVirtualAddress());
	commandList.SetGraphicsRootConstantBufferView(0U, frameCBufferGpuVAddress);
	commandList.SetGraphicsRootConstantBufferView(1U, frameCBufferGpuVAddress);
	commandList.SetGraphicsRootDescriptorTable(2U, mPixelShaderBuffersGpuDesc);

	// Draw object
	commandList.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList.DrawInstanced(6U, 1U, 0U, 0U);

	commandList.Close();

	CommandListExecutor::Get().AddCommandList(commandList);

	// Next frame
	currentFrameIndex = (currentFrameIndex + 1) % SettingsManager::sQueuedFrameCount;
}

bool AmbientOcclusionCmdListRecorder::ValidateData() const noexcept {
	for (std::uint32_t i = 0UL; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mFrameCBuffer[i] == nullptr) {
			return false;
		}
	}

	const bool result =
		mSampleKernelSize != 0U &&
		mSampleKernelBuffer != nullptr &&
		mSampleKernelBufferGpuDescBegin.ptr != 0UL &&
		mAmbientAccessibilityBufferCpuDesc.ptr != 0UL &&
		mPixelShaderBuffersGpuDesc.ptr != 0UL;

	return result;
}

void AmbientOcclusionCmdListRecorder::InitConstantBuffers() noexcept {
#ifdef _DEBUG
	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		ASSERT(mFrameCBuffer[i] == nullptr);
	}
#endif
	
	// Create frame cbuffers
	const std::size_t frameCBufferElemSize{ UploadBuffer::GetRoundedConstantBufferSizeInBytes(sizeof(FrameCBuffer)) };
	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		mFrameCBuffer[i] = &UploadBufferManager::CreateUploadBuffer(frameCBufferElemSize, 1U);
	}
}

void 
AmbientOcclusionCmdListRecorder::CreateSampleKernelBuffer(const void* randomSamples) noexcept {
	ASSERT(mSampleKernelBuffer == nullptr);
	ASSERT(randomSamples != nullptr);
	ASSERT(mSampleKernelSize != 0U);

	const std::size_t sampleKernelBufferElemSize{ sizeof(XMFLOAT4) };
	mSampleKernelBuffer = &UploadBufferManager::CreateUploadBuffer(sampleKernelBufferElemSize, mSampleKernelSize);
	const std::uint8_t* sampleKernelPtr = reinterpret_cast<const std::uint8_t*>(randomSamples);
	for (std::uint32_t i = 0UL; i < mSampleKernelSize; ++i) {
		mSampleKernelBuffer->CopyData(i, sampleKernelPtr + sampleKernelBufferElemSize * i, sampleKernelBufferElemSize);
	}
	mSampleKernelBufferGpuDescBegin.ptr = mSampleKernelBuffer->GetResource()->GetGPUVirtualAddress();
}

ID3D12Resource* AmbientOcclusionCmdListRecorder::CreateAndGetNoiseTexture(const void* noiseVectors) noexcept {
	ASSERT(noiseVectors != nullptr);
	// Kernel noise resource and fill it
	D3D12_RESOURCE_DESC resourceDescriptor = {};
	resourceDescriptor.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDescriptor.Alignment = 0U;
	resourceDescriptor.Width = mNoiseTextureDimension;
	resourceDescriptor.Height = mNoiseTextureDimension;
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
	noiseTexture = &ResourceManager::CreateCommittedResource(
		heapProps,
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
	noiseTextureUploadBuffer = &ResourceManager::CreateCommittedResource(
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		nullptr);

	return noiseTexture;
}

void AmbientOcclusionCmdListRecorder::InitShaderResourceViews(
	ID3D12Resource& normalSmoothnessBuffer,
	ID3D12Resource& depthBuffer,
	ID3D12Resource& noiseTexture) noexcept
{
	ASSERT(mSampleKernelBuffer != nullptr);
	ASSERT(mSampleKernelSize != 0U);

	ID3D12Resource* resources[] = {
		&normalSmoothnessBuffer,
		&depthBuffer,
		mSampleKernelBuffer->GetResource(),
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
	srvDescriptors[2].Format = mSampleKernelBuffer->GetResource()->GetDesc().Format;
	srvDescriptors[2].ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDescriptors[2].Buffer.FirstElement = 0UL;
	srvDescriptors[2].Buffer.NumElements = mSampleKernelSize;
	srvDescriptors[2].Buffer.StructureByteStride = sizeof(XMFLOAT4);

	// Fill kernel noise texture descriptor
	srvDescriptors[3].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDescriptors[3].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDescriptors[3].Texture2D.MostDetailedMip = 0;
	srvDescriptors[3].Texture2D.ResourceMinLODClamp = 0.0f;
	srvDescriptors[3].Format = noiseTexture.GetDesc().Format;
	srvDescriptors[3].Texture2D.MipLevels = noiseTexture.GetDesc().MipLevels;

	ASSERT(_countof(resources) == _countof(srvDescriptors));

	// Create SRVs
	mPixelShaderBuffersGpuDesc = 
		CbvSrvUavDescriptorManager::CreateShaderResourceViews(
			resources, 
			srvDescriptors, 
			_countof(srvDescriptors));
}
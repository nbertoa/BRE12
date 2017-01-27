#include "AmbientOcclusionCmdListRecorder.h"

#include <DirectXMath.h>

#include <CommandListExecutor\CommandListExecutor.h>
#include <CommandManager/CommandAllocatorManager.h>
#include <CommandManager/CommandListManager.h>
#include <DescriptorManager\CbvSrvUavDescriptorManager.h>
#include <DXUtils\d3dx12.h>
#include <MathUtils/MathUtils.h>
#include <PSOManager/PSOManager.h>
#include <ResourceManager/ResourceManager.h>
#include <ResourceStateManager\ResourceStateManager.h>
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
	const std::uint32_t sNoiseTextureDimension = 256U;

	void BuildCommandObjects(
		ID3D12GraphicsCommandList* &commandList, 
		ID3D12CommandAllocator* commandAllocators[], 
		const std::size_t commandAllocatorCount) noexcept 
	{
		ASSERT(commandList == nullptr);

#ifdef _DEBUG
		for (std::uint32_t i = 0U; i < commandAllocatorCount; ++i) {
			ASSERT(commandAllocators[i] == nullptr);
		}
#endif

		for (std::uint32_t i = 0U; i < commandAllocatorCount; ++i) {
			CommandAllocatorManager::Get().CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[i]);
		}

		CommandListManager::Get().CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *commandAllocators[0], commandList);

		// Start off in a closed state.  This is because the first time we refer 
		// to the command list we will Reset it, and it needs to be closed before
		// calling Reset.
		commandList->Close();
	}

	// Sample kernel for ambient occlusion. The requirements are that:
	// - Sample positions fall within the unit hemisphere
	// - Sample positions are more densely clustered towards the origin.
	//   This effectively attenuates the occlusion contribution
	//   according to distance from the kernel centre (samples closer
	//   to a point occlude it more than samples further away).
	void GenerateSampleKernel(const std::uint32_t numSamples, std::vector<XMFLOAT4>& kernels) {
		ASSERT(numSamples > 0U);

		kernels.resize(numSamples);
		XMFLOAT4* data(kernels.data());
		XMVECTOR vec;
		const float numSamplesF = static_cast<float>(numSamples);
		for (std::uint32_t i = 0U; i < numSamples; ++i) {
			XMFLOAT4& elem = data[i];

			// Create sample points on the surface of a hemisphere
			// oriented along the z axis
			const float x = MathUtils::RandomFloatInInverval(-1.0f, 1.0f);
			const float y = MathUtils::RandomFloatInInverval(-1.0f, 1.0f);
			const float z = MathUtils::RandomFloatInInverval(-1.0f, 0.0f);
			elem = XMFLOAT4(x, y, z, 0.0f);
			vec = XMLoadFloat4(&elem);
			vec = XMVector4Normalize(vec);

			// Accelerating interpolation function to falloff 
			// from the distance from the origin.
			float scale = i / numSamplesF;
			scale = MathUtils::Lerp(0.5f, 1.0f, scale);
			vec = XMVectorScale(vec, scale);
			XMStoreFloat4(&elem, vec);
		}
	}

	void GenerateSampleKernel(std::vector<XMFLOAT4>& kernels) {
		kernels.resize(14);

		// Start with 14 uniformly distributed vectors.  We choose the 8 corners of the cube
		// and the 6 center points along each cube face.  We always alternate the points on 
		// opposites sides of the cubes.  This way we still get the vectors spread out even
		// if we choose to use less than 14 samples.

		// 8 cube corners
		kernels[0] = XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
		kernels[1] = XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);

		kernels[2] = XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
		kernels[3] = XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);

		kernels[4] = XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
		kernels[5] = XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);

		kernels[6] = XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);
		kernels[7] = XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);

		// 6 centers of cube faces
		kernels[8] = XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
		kernels[9] = XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);

		kernels[10] = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
		kernels[11] = XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);

		kernels[12] = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
		kernels[13] = XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);

		for (int i = 0; i < 14; ++i) {
			// Create random lengths in [0.25, 1.0].
			const float s = MathUtils::RandomFloatInInverval(0.25f, 1.0f);

			XMVECTOR v = s * XMVector4Normalize(XMLoadFloat4(&kernels[i]));

			XMStoreFloat4(&kernels[i], v);
		}
	}

	// Generate a set of random values used to rotate the sample kernel,
	// which will effectively increase the sample count and minimize 
	// the 'banding' artifacts.
	void GenerateNoise(const std::uint32_t numSamples, std::vector<XMFLOAT4>& noises) {
		ASSERT(numSamples > 0U);

		noises.resize(numSamples);
		XMFLOAT4* data(noises.data());
		XMVECTOR vec;
		for (std::uint32_t i = 0U; i < numSamples; ++i) {
			XMFLOAT4& elem = data[i];

			// Create sample points on the surface of a hemisphere
			// oriented along the z axis
			const float x = MathUtils::RandomFloatInInverval(-1.0f, 1.0f);
			const float y = MathUtils::RandomFloatInInverval(-1.0f, 1.0f);
			const float z = 0.0f;			
			elem = XMFLOAT4(x, y, z, 0.0f);
			vec = XMLoadFloat4(&elem);
			vec = XMVector4Normalize(vec);
			XMStoreFloat4(&elem, vec);

			// Map from [-1.0f, 1.0f] to [0.0f, 1.0f]
			elem.x = elem.x * 0.5f + 0.5f;
			elem.y = elem.y * 0.5f + 0.5f;
			elem.z = elem.z * 0.5f + 0.5f;
		}
	}

	void GenerateNoise(std::vector<XMFLOAT4>& noises) {
		const std::size_t numElems = sNoiseTextureDimension * sNoiseTextureDimension;
		noises.resize(numElems);
		XMFLOAT4* data(noises.data());
		for (std::uint32_t i = 0U; i < numElems; ++i) {
			XMFLOAT4& elem = data[i];

			// Create sample points on the surface of a hemisphere
			// oriented along the z axis
			const float x = MathUtils::RandomFloatInInverval(0.0f, 1.0f);
			const float y = MathUtils::RandomFloatInInverval(0.0f, 1.0f);
			const float z = MathUtils::RandomFloatInInverval(0.0f, 1.0f);
			elem = XMFLOAT4(x, y, z, 0.0f);
		}
	}
}

AmbientOcclusionCmdListRecorder::AmbientOcclusionCmdListRecorder() {
	BuildCommandObjects(mCommandList, mCommandAllocators, _countof(mCommandAllocators));
}

void AmbientOcclusionCmdListRecorder::InitPSO() noexcept {
	ASSERT(sPSO == nullptr);
	ASSERT(sRootSignature == nullptr);

	// Build pso and root signature
	PSOManager::PSOCreationData psoData{};
	const std::size_t renderTargetCount{ _countof(psoData.mRtFormats) };
	psoData.mBlendDesc = D3DFactory::GetAlwaysBlendDesc();
	psoData.mDepthStencilDesc = D3DFactory::GetDisabledDepthStencilDesc();
	psoData.mPSFilename = "AmbientLightPass/Shaders/AmbientOcclusion/PS.cso";
	psoData.mRootSignFilename = "AmbientLightPass/Shaders/AmbientOcclusion/RS.cso";
	psoData.mVSFilename = "AmbientLightPass/Shaders/AmbientOcclusion/VS.cso";
	psoData.mNumRenderTargets = 1U;
	psoData.mRtFormats[0U] = DXGI_FORMAT_R16_UNORM;
	for (std::size_t i = psoData.mNumRenderTargets; i < renderTargetCount; ++i) {
		psoData.mRtFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	psoData.mTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PSOManager::Get().CreateGraphicsPSO(psoData, sPSO, sRootSignature);

	ASSERT(sPSO != nullptr);
	ASSERT(sRootSignature != nullptr);
}

void AmbientOcclusionCmdListRecorder::Init(
	ID3D12Resource& normalSmoothnessBuffer,	
	const D3D12_CPU_DESCRIPTOR_HANDLE& ambientAccessBufferCpuDesc,
	ID3D12Resource& depthBuffer) noexcept
{
	ASSERT(ValidateData() == false);

	mAmbientAccessBufferCpuDesc = ambientAccessBufferCpuDesc;

	mNumSamples = 128U;
	std::vector<XMFLOAT4> sampleKernel;
	GenerateSampleKernel(mNumSamples, sampleKernel);
	std::vector<XMFLOAT4> noises;
	GenerateNoise(mNumSamples, noises);
	BuildBuffers(sampleKernel.data(), noises.data(), normalSmoothnessBuffer, depthBuffer);

	ASSERT(ValidateData());
}

void AmbientOcclusionCmdListRecorder::RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(ValidateData());
	ASSERT(sPSO != nullptr);
	ASSERT(sRootSignature != nullptr);

	static std::uint32_t currentFrameIndex = 0U;

	ID3D12CommandAllocator* commandAllocator{ mCommandAllocators[currentFrameIndex] };
	ASSERT(commandAllocator != nullptr);
	
	CHECK_HR(commandAllocator->Reset());
	CHECK_HR(mCommandList->Reset(commandAllocator, sPSO));

	// Update frame constants
	UploadBuffer& uploadFrameCBuffer(*mFrameCBuffer[currentFrameIndex]);
	uploadFrameCBuffer.CopyData(0U, &frameCBuffer, sizeof(frameCBuffer));

	mCommandList->RSSetViewports(1U, &SettingsManager::sScreenViewport);
	mCommandList->RSSetScissorRects(1U, &SettingsManager::sScissorRect);
	mCommandList->OMSetRenderTargets(1U, &mAmbientAccessBufferCpuDesc, false, nullptr);

	ID3D12DescriptorHeap* heaps[] = { &CbvSrvUavDescriptorManager::Get().GetDescriptorHeap() };
	mCommandList->SetDescriptorHeaps(_countof(heaps), heaps);
	mCommandList->SetGraphicsRootSignature(sRootSignature);	

	// Set root parameters
	const D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(uploadFrameCBuffer.GetResource()->GetGPUVirtualAddress());
	mCommandList->SetGraphicsRootConstantBufferView(0U, frameCBufferGpuVAddress);
	mCommandList->SetGraphicsRootConstantBufferView(1U, frameCBufferGpuVAddress);
	mCommandList->SetGraphicsRootDescriptorTable(2U, mPixelShaderBuffersGpuDesc);

	// Draw object
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCommandList->DrawInstanced(6U, 1U, 0U, 0U);

	mCommandList->Close();

	CommandListExecutor::Get().AddCommandList(*mCommandList);

	// Next frame
	currentFrameIndex = (currentFrameIndex + 1) % SettingsManager::sQueuedFrameCount;
}

bool AmbientOcclusionCmdListRecorder::ValidateData() const noexcept {
	for (std::uint32_t i = 0UL; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCommandAllocators[i] == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0UL; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mFrameCBuffer[i] == nullptr) {
			return false;
		}
	}

	const bool result =
		mCommandList != nullptr &&
		mNumSamples != 0U &&
		mSampleKernelBuffer != nullptr &&
		mSampleKernelBufferGpuDescBegin.ptr != 0UL &&
		mAmbientAccessBufferCpuDesc.ptr != 0UL &&
		mPixelShaderBuffersGpuDesc.ptr != 0UL;

	return result;
}

void AmbientOcclusionCmdListRecorder::BuildBuffers(
	const void* sampleKernel, 
	const void* kernelNoise,
	ID3D12Resource& normalSmoothnessBuffer,
	ID3D12Resource& depthBuffer) noexcept 
{
#ifdef _DEBUG
	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		ASSERT(mFrameCBuffer[i] == nullptr);
	}
#endif
	ASSERT(mSampleKernelBuffer == nullptr);
	ASSERT(sampleKernel != nullptr);
	ASSERT(kernelNoise != nullptr);
	ASSERT(mNumSamples != 0U);

	// Create sample kernel buffer and fill it
	const std::size_t sampleKernelBufferElemSize{ sizeof(XMFLOAT4) };
	ResourceManager::Get().CreateUploadBuffer(sampleKernelBufferElemSize, mNumSamples, mSampleKernelBuffer);
	const std::uint8_t* sampleKernelPtr = reinterpret_cast<const std::uint8_t*>(sampleKernel);
	for (std::uint32_t i = 0UL; i < mNumSamples; ++i) {
		mSampleKernelBuffer->CopyData(i, sampleKernelPtr + sampleKernelBufferElemSize * i, sampleKernelBufferElemSize);
	}
	mSampleKernelBufferGpuDescBegin.ptr = mSampleKernelBuffer->GetResource()->GetGPUVirtualAddress();

	// Kernel noise resource and fill it
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Alignment = 0U;
	resDesc.Width = sNoiseTextureDimension;
	resDesc.Height = sNoiseTextureDimension;
	resDesc.DepthOrArraySize = 1U;
	resDesc.MipLevels = 1U;
	resDesc.SampleDesc.Count = 1U;
	resDesc.SampleDesc.Quality = 0U;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	ID3D12Resource* noiseTexture{ nullptr };
	CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
	ResourceManager::Get().CreateCommittedResource(heapProps, D3D12_HEAP_FLAG_NONE, resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, noiseTexture);

	// In order to copy CPU memory data into our default buffer, we need to create
	// an intermediate upload heap. 
	const std::uint32_t num2DSubresources = resDesc.DepthOrArraySize * resDesc.MipLevels;
	const std::size_t uploadBufferSize = GetRequiredIntermediateSize(noiseTexture, 0, num2DSubresources);
	ID3D12Resource* noiseTextureUploadBuffer{ nullptr };
	ResourceManager::Get().CreateCommittedResource(
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		noiseTextureUploadBuffer);

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = kernelNoise;
	subResourceData.RowPitch = sNoiseTextureDimension * sizeof(XMFLOAT4);
	subResourceData.SlicePitch = subResourceData.RowPitch * sNoiseTextureDimension;
	
	//
	// Schedule to copy the data to the default resource, and change states.
	// Note that mCurrSol is put in the GENERIC_READ state so it can be 
	// read by a shader.
	//

	/*ID3D12CommandAllocator* commandAllocators{ mCommandAllocators[0] };
	ASSERT(commandAllocators != nullptr);
	CHECK_HR(commandAllocators->Reset());
	CHECK_HR(mCommandList->Reset(commandAllocators, nullptr));

	D3D12_RESOURCE_BARRIER barriers[] = { 
		ResourceStateManager::Get().TransitionState(*noiseTexture, D3D12_RESOURCE_STATE_COPY_DEST),		
	};
	mCommandList->ResourceBarrier(_countof(barriers), barriers);
	UpdateSubresources(mCommandList, noiseTexture, noiseTextureUploadBuffer, 0U, 0U, num2DSubresources, &subResourceData);
	barriers[0] = ResourceStateManager::Get().TransitionState(*noiseTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	mCommandList->ResourceBarrier(_countof(barriers), barriers);

	mCommandList->Close();
	mCmdListQueue.push(mCommandList);*/

	// Create frame cbuffers
	const std::size_t frameCBufferElemSize{ UploadBuffer::RoundConstantBufferSizeInBytes(sizeof(FrameCBuffer)) };
	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		ResourceManager::Get().CreateUploadBuffer(frameCBufferElemSize, 1U, mFrameCBuffer[i]);
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc[4U]{};
	ID3D12Resource* res[4] = {
		&normalSmoothnessBuffer,
		&depthBuffer,
		mSampleKernelBuffer->GetResource(),
		noiseTexture,
	};

	// Fill normal_smoothness buffer texture descriptor
	srvDesc[0].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc[0].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc[0].Texture2D.MostDetailedMip = 0;
	srvDesc[0].Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc[0].Format = normalSmoothnessBuffer.GetDesc().Format;
	srvDesc[0].Texture2D.MipLevels = normalSmoothnessBuffer.GetDesc().MipLevels;
	
	// Fill depth buffer descriptor
	srvDesc[1].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc[1].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc[1].Texture2D.MostDetailedMip = 0;
	srvDesc[1].Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc[1].Format = SettingsManager::sDepthStencilSRVFormat;
	srvDesc[1].Texture2D.MipLevels = depthBuffer.GetDesc().MipLevels;

	// Fill sample kernel buffer descriptor
	srvDesc[2].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc[2].Format = mSampleKernelBuffer->GetResource()->GetDesc().Format;
	srvDesc[2].ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc[2].Buffer.FirstElement = 0UL;
	srvDesc[2].Buffer.NumElements = mNumSamples;
	srvDesc[2].Buffer.StructureByteStride = sizeof(XMFLOAT4);

	// Fill kernel noise texture descriptor
	srvDesc[3].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc[3].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc[3].Texture2D.MostDetailedMip = 0;
	srvDesc[3].Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc[3].Format = noiseTexture->GetDesc().Format;
	srvDesc[3].Texture2D.MipLevels = noiseTexture->GetDesc().MipLevels;

	// Create SRVs
	mPixelShaderBuffersGpuDesc = CbvSrvUavDescriptorManager::Get().CreateShaderResourceViews(res, srvDesc, _countof(srvDesc));
}
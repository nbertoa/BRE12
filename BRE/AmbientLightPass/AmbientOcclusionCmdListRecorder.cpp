#include "AmbientOcclusionCmdListRecorder.h"

#include <DirectXMath.h>

#include <CommandManager/CommandManager.h>
#include <DescriptorManager\DescriptorManager.h>
#include <DXUtils\d3dx12.h>
#include <MathUtils/MathUtils.h>
#include <PSOCreator/PSOCreator.h>
#include <ResourceManager/ResourceManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils/DebugUtils.h>

using namespace DirectX;

// Root Signature:
// "CBV(b0, visibility = SHADER_VISIBILITY_VERTEX), " \ 0 -> Frame CBuffer
// "CBV(b0, visibility = SHADER_VISIBILITY_PIXEL), " \ 1 -> Frame CBuffer
// "DescriptorTable(SRV(t0), SRV(t1), SRV(t2), SRV(t3), visibility = SHADER_VISIBILITY_PIXEL)" 2 -> normal_smoothness + depth + sample kernel + kernel noise

namespace {
	ID3D12PipelineState* sPSO{ nullptr };
	ID3D12RootSignature* sRootSign{ nullptr };
	const std::uint32_t sNoiseTextureDimension = 4U;

	void BuildCommandObjects(ID3D12GraphicsCommandList* &cmdList, ID3D12CommandAllocator* cmdAlloc[], const std::size_t cmdAllocCount) noexcept {
		ASSERT(cmdList == nullptr);

#ifdef _DEBUG
		for (std::uint32_t i = 0U; i < cmdAllocCount; ++i) {
			ASSERT(cmdAlloc[i] == nullptr);
		}
#endif

		for (std::uint32_t i = 0U; i < cmdAllocCount; ++i) {
			CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc[i]);
		}

		CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAlloc[0], cmdList);

		// Start off in a closed state.  This is because the first time we refer 
		// to the command list we will Reset it, and it needs to be closed before
		// calling Reset.
		cmdList->Close();
	}

	// Sample kernel for ambient occlusion. The requirements are that:
	// - Sample positions fall within the unit hemisphere
	// - Sample positions are more densely clustered towards the origin.
	//   This effectively attenuates the occlusion contribution
	//   according to distance from the kernel centre (samples closer
	//   to a point occlude it more than samples further away).
	void GenerateSampleKernel(const std::uint32_t numSamples, std::vector<XMFLOAT3>& kernels) {
		ASSERT(numSamples > 0U);

		kernels.resize(numSamples);
		XMFLOAT3* data(kernels.data());
		XMVECTOR vec;
		const float numSamplesF = static_cast<float>(numSamples);
		for (std::uint32_t i = 0U; i < numSamples; ++i) {
			XMFLOAT3& elem = data[i];

			// Create sample points on the surface of a hemisphere
			// oriented along the z axis
			const float x = MathUtils::RandF(-1.0f, 1.0f);
			const float y = MathUtils::RandF(-1.0f, 1.0f);
			const float z = MathUtils::RandF(0.f, 1.0f);
			elem = XMFLOAT3(x, y, z);
			vec = XMLoadFloat3(&elem);
			vec = XMVector3Normalize(vec);

			// Accelerating interpolation function to falloff 
			// from the distance from the origin.
			float scale = i / numSamplesF;
			scale = MathUtils::Lerp(0.1f, 1.0f, scale * scale);
			vec = XMVectorScale(vec, scale);
			XMStoreFloat3(&elem, vec);
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
			const float x = MathUtils::RandF(-1.0f, 1.0f);
			const float y = MathUtils::RandF(-1.0f, 1.0f);
			const float z = 0.0f;
			XMFLOAT3 mappedVec = MathUtils::MapF1(XMFLOAT3(x, y, z));
			elem = XMFLOAT4(mappedVec.x, mappedVec.y, mappedVec.z, 0.0f);
			vec = XMLoadFloat4(&elem);
			vec = XMVector4Normalize(vec);
			XMStoreFloat4(&elem, vec);
		}
	}
}

AmbientOcclusionCmdListRecorder::AmbientOcclusionCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue)
	: mDevice(device)
	, mCmdListQueue(cmdListQueue)
{
	BuildCommandObjects(mCmdList, mCmdAlloc, _countof(mCmdAlloc));
}

void AmbientOcclusionCmdListRecorder::InitPSO() noexcept {
	ASSERT(sPSO == nullptr);
	ASSERT(sRootSign == nullptr);

	// Build pso and root signature
	PSOCreator::PSOParams psoParams{};
	const std::size_t rtCount{ _countof(psoParams.mRtFormats) };
	psoParams.mBlendDesc = D3DFactory::AlwaysBlendDesc();
	psoParams.mDepthStencilDesc = D3DFactory::DisableDepthStencilDesc();
	psoParams.mInputLayout = D3DFactory::PosNormalTangentTexCoordInputLayout();
	psoParams.mPSFilename = "AmbientLightPass/Shaders/AmbientOcclusion/PS.cso";
	psoParams.mRootSignFilename = "AmbientLightPass/Shaders/AmbientOcclusion/RS.cso";
	psoParams.mVSFilename = "AmbientLightPass/Shaders/AmbientOcclusion/VS.cso";
	psoParams.mNumRenderTargets = 1U;
	psoParams.mRtFormats[0U] = DXGI_FORMAT_R16_UNORM;
	for (std::size_t i = psoParams.mNumRenderTargets; i < rtCount; ++i) {
		psoParams.mRtFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	psoParams.mTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PSOCreator::CreatePSO(psoParams, sPSO, sRootSign);

	ASSERT(sPSO != nullptr);
	ASSERT(sRootSign != nullptr);
}

void AmbientOcclusionCmdListRecorder::Init(
	const BufferCreator::VertexBufferData& vertexBufferData,
	const BufferCreator::IndexBufferData& indexBufferData,
	ID3D12Resource& normalSmoothnessBuffer,	
	const D3D12_CPU_DESCRIPTOR_HANDLE& ambientAccessBufferCpuDesc,
	ID3D12Resource& depthBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept
{
	ASSERT(ValidateData() == false);

	mVertexBufferData = vertexBufferData;
	mIndexBufferData = indexBufferData;
	mAmbientAccessBufferCpuDesc = ambientAccessBufferCpuDesc;
	mDepthBufferCpuDesc = depthBufferCpuDesc;

	mNumSamples = 128U;
	std::vector<XMFLOAT3> sampleKernel;
	GenerateSampleKernel(mNumSamples, sampleKernel);
	std::vector<XMFLOAT4> noises;
	GenerateNoise(sNoiseTextureDimension * sNoiseTextureDimension, noises);
	BuildBuffers(sampleKernel.data(), noises.data(), normalSmoothnessBuffer, depthBuffer);

	ASSERT(ValidateData());
}

void AmbientOcclusionCmdListRecorder::RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(ValidateData());
	ASSERT(sPSO != nullptr);
	ASSERT(sRootSign != nullptr);

	ID3D12CommandAllocator* cmdAlloc{ mCmdAlloc[mCurrFrameIndex] };
	ASSERT(cmdAlloc != nullptr);
	
	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCmdList->Reset(cmdAlloc, sPSO));

	// Update frame constants
	UploadBuffer& uploadFrameCBuffer(*mFrameCBuffer[mCurrFrameIndex]);
	uploadFrameCBuffer.CopyData(0U, &frameCBuffer, sizeof(frameCBuffer));

	mCmdList->RSSetViewports(1U, &Settings::sScreenViewport);
	mCmdList->RSSetScissorRects(1U, &Settings::sScissorRect);
	mCmdList->OMSetRenderTargets(1U, &mAmbientAccessBufferCpuDesc, false, &mDepthBufferCpuDesc);

	ID3D12DescriptorHeap* heaps[] = { &DescriptorManager::Get().GetCbvSrcUavDescriptorHeap() };
	mCmdList->SetDescriptorHeaps(_countof(heaps), heaps);
	mCmdList->SetGraphicsRootSignature(sRootSign);
	
	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set root parameters
	const D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(uploadFrameCBuffer.Resource()->GetGPUVirtualAddress());
	mCmdList->SetGraphicsRootConstantBufferView(0U, frameCBufferGpuVAddress);
	mCmdList->SetGraphicsRootConstantBufferView(1U, frameCBufferGpuVAddress);
	mCmdList->SetGraphicsRootDescriptorTable(2U, mPixelShaderBuffersGpuDescHandle);

	// Draw object
	mCmdList->IASetVertexBuffers(0U, 1U, &mVertexBufferData.mBufferView);
	mCmdList->IASetIndexBuffer(&mIndexBufferData.mBufferView);
	mCmdList->DrawIndexedInstanced(mIndexBufferData.mCount, 1U, 0U, 0U, 0U);

	mCmdList->Close();

	mCmdListQueue.push(mCmdList);

	// Next frame
	mCurrFrameIndex = (mCurrFrameIndex + 1) % Settings::sQueuedFrameCount;
}

bool AmbientOcclusionCmdListRecorder::ValidateData() const noexcept {
	for (std::uint32_t i = 0UL; i < Settings::sQueuedFrameCount; ++i) {
		if (mCmdAlloc[i] == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0UL; i < Settings::sQueuedFrameCount; ++i) {
		if (mFrameCBuffer[i] == nullptr) {
			return false;
		}
	}

	const bool result =
		mCmdList != nullptr &&
		mNumSamples != 0U &&
		mSampleKernelBuffer != nullptr &&
		mSampleKernelBufferGpuDescHandleBegin.ptr != 0UL &&
		mAmbientAccessBufferCpuDesc.ptr != 0UL &&
		mDepthBufferCpuDesc.ptr != 0UL &&
		mPixelShaderBuffersGpuDescHandle.ptr != 0UL;

	return result;
}

void AmbientOcclusionCmdListRecorder::BuildBuffers(
	const void* sampleKernel, 
	const void* kernelNoise,
	ID3D12Resource& normalSmoothnessBuffer,
	ID3D12Resource& depthBuffer) noexcept {

#ifdef _DEBUG
	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		ASSERT(mFrameCBuffer[i] == nullptr);
	}
#endif
	ASSERT(mSampleKernelBuffer == nullptr);
	ASSERT(sampleKernel != nullptr);
	ASSERT(kernelNoise != nullptr);
	ASSERT(mNumSamples != 0U);

	// Create sample kernel buffer and fill it
	const std::size_t sampleKernelBufferElemSize{ sizeof(XMFLOAT3) };
	ResourceManager::Get().CreateUploadBuffer(sampleKernelBufferElemSize, mNumSamples, mSampleKernelBuffer);
	const std::uint8_t* sampleKernelPtr = reinterpret_cast<const std::uint8_t*>(sampleKernel);
	for (std::uint32_t i = 0UL; i < mNumSamples; ++i) {
		mSampleKernelBuffer->CopyData(i, sampleKernelPtr + sizeof(XMFLOAT3) * i, sizeof(XMFLOAT3));
	}
	mSampleKernelBufferGpuDescHandleBegin.ptr = mSampleKernelBuffer->Resource()->GetGPUVirtualAddress();

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

	ID3D12CommandAllocator* cmdAlloc{ mCmdAlloc[mCurrFrameIndex] };
	ASSERT(cmdAlloc != nullptr);
	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCmdList->Reset(cmdAlloc, nullptr));

	D3D12_RESOURCE_BARRIER barriers[] = { CD3DX12_RESOURCE_BARRIER::Transition(noiseTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST) };
	mCmdList->ResourceBarrier(_countof(barriers), barriers);
	UpdateSubresources(mCmdList, noiseTexture, noiseTextureUploadBuffer, 0U, 0U, num2DSubresources, &subResourceData);
	barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(noiseTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	mCmdList->ResourceBarrier(_countof(barriers), barriers);

	mCmdList->Close();
	mCmdListQueue.push(mCmdList);

	// Create frame cbuffers
	const std::size_t frameCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(FrameCBuffer)) };
	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		ResourceManager::Get().CreateUploadBuffer(frameCBufferElemSize, 1U, mFrameCBuffer[i]);
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc[4U]{};
	ID3D12Resource* res[4] = {
		&normalSmoothnessBuffer,
		&depthBuffer,
		mSampleKernelBuffer->Resource(),
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
	srvDesc[1].Format = Settings::sDepthStencilSRVFormat;
	srvDesc[1].Texture2D.MipLevels = depthBuffer.GetDesc().MipLevels;

	// Fill sample kernel buffer descriptor
	srvDesc[2].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc[2].Format = mSampleKernelBuffer->Resource()->GetDesc().Format;
	srvDesc[2].ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc[2].Buffer.FirstElement = 0UL;
	srvDesc[2].Buffer.NumElements = mNumSamples;
	srvDesc[2].Buffer.StructureByteStride = sizeof(XMFLOAT3);

	// Fill kernel noise texture descriptor
	srvDesc[3].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc[3].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc[3].Texture2D.MostDetailedMip = 0;
	srvDesc[3].Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc[3].Format = noiseTexture->GetDesc().Format;
	srvDesc[3].Texture2D.MipLevels = noiseTexture->GetDesc().MipLevels;

	// Create SRVs
	mPixelShaderBuffersGpuDescHandle = DescriptorManager::Get().CreateShaderResourceView(res, srvDesc, _countof(srvDesc));
}
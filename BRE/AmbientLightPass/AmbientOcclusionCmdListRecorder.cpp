#include "AmbientOcclusionCmdListRecorder.h"

#include <DirectXMath.h>

#include <CommandManager/CommandManager.h>
#include <DXUtils\d3dx12.h>
#include <MathUtils/MathUtils.h>
#include <PSOCreator/PSOCreator.h>
#include <ResourceManager/ResourceManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils/DebugUtils.h>

// Root Signature:
// "CBV(b0, visibility = SHADER_VISIBILITY_VERTEX), " \ 0 -> Frame CBuffer
// "CBV(b0, visibility = SHADER_VISIBILITY_PIXEL), " \ 1 -> Frame CBuffer
// "DescriptorTable(SRV(t0), SRV(t1), SRV(t2), SRV(t3), visibility = SHADER_VISIBILITY_PIXEL)" 2 -> normal_smoothness + depth + sample kernel + kernel noise

namespace {
	ID3D12PipelineState* sPSO{ nullptr };
	ID3D12RootSignature* sRootSign{ nullptr };

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
	void GenerateSampleKernel(const std::uint32_t numSamples, std::vector<DirectX::XMFLOAT3>& kernels) {
		ASSERT(numSamples > 0U);

		kernels.resize(numSamples);
		DirectX::XMFLOAT3* data(kernels.data());
		DirectX::XMVECTOR vec;
		const float numSamplesF = static_cast<float>(numSamples);
		for (std::uint32_t i = 0U; i < numSamples; ++i) {
			DirectX::XMFLOAT3& elem = data[i];

			// Create sample points on the surface of a hemisphere
			// oriented along the z axis
			const float x = MathUtils::RandF(-1.0f, 1.0f);
			const float y = MathUtils::RandF(-1.0f, 1.0f);
			const float z = MathUtils::RandF(-1.0f, 1.0f);
			elem = DirectX::XMFLOAT3(x, y, z);
			vec = DirectX::XMLoadFloat3(&elem);
			vec = DirectX::XMVector3Normalize(vec);

			// Accelerating interpolation function to falloff 
			// from the distance from the origin.
			float scale = i / numSamplesF;
			scale = MathUtils::Lerp(0.1f, 1.0f, scale * scale);
			vec = DirectX::XMVectorScale(vec, scale);
			DirectX::XMStoreFloat3(&elem, vec);
		}
	}

	// Generate a set of random values used to rotate the sample kernel,
	// which will effectively increase the sample count and minimize 
	// the 'banding' artifacts.
	void GenerateNoise(const std::uint32_t numSamples, std::vector<DirectX::XMFLOAT3>& noises) {
		ASSERT(numSamples > 0U);

		noises.resize(numSamples);
		DirectX::XMFLOAT3* data(noises.data());
		DirectX::XMVECTOR vec;
		for (std::uint32_t i = 0U; i < numSamples; ++i) {
			DirectX::XMFLOAT3& elem = data[i];

			// Create sample points on the surface of a hemisphere
			// oriented along the z axis
			const float x = MathUtils::RandF(-1.0f, 1.0f);
			const float y = MathUtils::RandF(-1.0f, 1.0f);
			const float z = MathUtils::RandF(0.0f, 1.0f);
			elem = DirectX::XMFLOAT3(x, y, z);
			vec = DirectX::XMLoadFloat3(&elem);
			vec = DirectX::XMVector3Normalize(vec);
			DirectX::XMStoreFloat3(&elem, vec);
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
	std::vector<DirectX::XMFLOAT3> sampleKernel;
	GenerateSampleKernel(mNumSamples, sampleKernel);
	std::vector<DirectX::XMFLOAT3> noises;
	GenerateNoise(mNumSamples, noises);
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

	mCmdList->SetDescriptorHeaps(1U, &mCbvSrvUavDescHeap);
	mCmdList->SetGraphicsRootSignature(sRootSign);
	
	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set root parameters
	const D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(uploadFrameCBuffer.Resource()->GetGPUVirtualAddress());
	mCmdList->SetGraphicsRootConstantBufferView(0U, frameCBufferGpuVAddress);
	mCmdList->SetGraphicsRootConstantBufferView(1U, frameCBufferGpuVAddress);
	mCmdList->SetGraphicsRootDescriptorTable(2U, mCbvSrvUavDescHeap->GetGPUDescriptorHandleForHeapStart());

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
		mCbvSrvUavDescHeap != nullptr &&
		mNumSamples != 0U &&
		mSampleKernelBuffer != nullptr &&
		mSampleKernelBufferGpuDescHandleBegin.ptr != 0UL &&
		mAmbientAccessBufferCpuDesc.ptr != 0UL &&
		mDepthBufferCpuDesc.ptr != 0UL;

	return result;
}

void AmbientOcclusionCmdListRecorder::BuildBuffers(
	const void* sampleKernel, 
	const void* kernelNoise,
	ID3D12Resource& normalSmoothnessBuffer,
	ID3D12Resource& depthBuffer) noexcept {

	ASSERT(mCbvSrvUavDescHeap == nullptr);
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
	const std::size_t sampleKernelBufferElemSize{ sizeof(DirectX::XMFLOAT3) };
	ResourceManager::Get().CreateUploadBuffer(sampleKernelBufferElemSize, mNumSamples, mSampleKernelBuffer);
	const std::uint8_t* sampleKernelPtr = reinterpret_cast<const std::uint8_t*>(sampleKernel);
	for (std::uint32_t i = 0UL; i < mNumSamples; ++i) {
		mSampleKernelBuffer->CopyData(i, sampleKernelPtr + sizeof(DirectX::XMFLOAT3) * i, sizeof(DirectX::XMFLOAT3));
	}
	mSampleKernelBufferGpuDescHandleBegin.ptr = mSampleKernelBuffer->Resource()->GetGPUVirtualAddress();

	// Kernel noise resource and fill it
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Alignment = 0U;
	resDesc.Width = mNumSamples;
	resDesc.Height = mNumSamples;
	resDesc.DepthOrArraySize = 1U;
	resDesc.MipLevels = 0U;
	resDesc.SampleDesc.Count = 1U;
	resDesc.SampleDesc.Quality = 0U;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
	ID3D12Resource* res{ nullptr };
	CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
	ResourceManager::Get().CreateCommittedResource(heapProps, D3D12_HEAP_FLAG_NONE, resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, res);
	/*std::uint8_t* data{ nullptr };
	CHECK_HR(res->Map(0, nullptr, reinterpret_cast<void**>(&data)));
	memcpy(data, kernelNoise, sizeof(DirectX::XMFLOAT3) * mNumSamples);
	res->Unmap(0, nullptr);*/

	// Create frame cbuffers
	const std::size_t frameCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(FrameCBuffer)) };
	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		ResourceManager::Get().CreateUploadBuffer(frameCBufferElemSize, 1U, mFrameCBuffer[i]);
	}

	// Create CBV_SRV_UAV cbuffer descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0U;
	descHeapDesc.NumDescriptors = 4U; // normal_smoothness buffer + depth buffer + sample kernel buffer + kernel noise buffer
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ResourceManager::Get().CreateDescriptorHeap(descHeapDesc, mCbvSrvUavDescHeap);

	// Create normal_smoothness buffer texture descriptor
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = normalSmoothnessBuffer.GetDesc().Format;
	srvDesc.Texture2D.MipLevels = normalSmoothnessBuffer.GetDesc().MipLevels;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuDescHandle = mCbvSrvUavDescHeap->GetCPUDescriptorHandleForHeapStart();
	ResourceManager::Get().CreateShaderResourceView(normalSmoothnessBuffer, srvDesc, cpuDescHandle);
	
	// Create depth buffer descriptor
	srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = Settings::sDepthStencilSRVFormat;
	srvDesc.Texture2D.MipLevels = depthBuffer.GetDesc().MipLevels;
	const std::size_t descHandleIncSize{ ResourceManager::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	cpuDescHandle.ptr += descHandleIncSize;
	ResourceManager::Get().CreateShaderResourceView(depthBuffer, srvDesc, cpuDescHandle);

	// Create sample kernel buffer descriptor
	srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mSampleKernelBuffer->Resource()->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0UL;
	srvDesc.Buffer.NumElements = mNumSamples;
	srvDesc.Buffer.StructureByteStride = sizeof(DirectX::XMFLOAT3);	
	cpuDescHandle.ptr += descHandleIncSize;
	ResourceManager::Get().CreateShaderResourceView(*mSampleKernelBuffer->Resource(), srvDesc, cpuDescHandle);

	// Create kernel noise texture descriptor
	srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = normalSmoothnessBuffer.GetDesc().Format;
	srvDesc.Texture2D.MipLevels = normalSmoothnessBuffer.GetDesc().MipLevels;
	cpuDescHandle.ptr += descHandleIncSize;
	ResourceManager::Get().CreateShaderResourceView(normalSmoothnessBuffer, srvDesc, mCbvSrvUavDescHeap->GetCPUDescriptorHandleForHeapStart());
}
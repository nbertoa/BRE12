#include "EnvironmentLightCmdListRecorder.h"

#include <DirectXMath.h>

#include <CommandManager/CommandManager.h>
#include <PSOCreator/PSOCreator.h>
#include <ResourceManager/ResourceManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils/DebugUtils.h>

// Root Signature:
// "CBV(b0, visibility = SHADER_VISIBILITY_VERTEX), " \ 0 -> Frame CBuffer
// "CBV(b0, visibility = SHADER_VISIBILITY_PIXEL), " \ 1 -> Frame CBuffer
// "CBV(b1, visibility = SHADER_VISIBILITY_PIXEL), " \ 2 -> Immutable CBuffer
// "DescriptorTable(SRV(t0), SRV(t1), SRV(t2), SRV(t3), SRV(t4), visibility = SHADER_VISIBILITY_PIXEL), " \ 2 -> Textures 

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
}

EnvironmentLightCmdListRecorder::EnvironmentLightCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue)
	: mDevice(device)
	, mCmdListQueue(cmdListQueue)
{
	BuildCommandObjects(mCmdList, mCmdAlloc, _countof(mCmdAlloc));
}

void EnvironmentLightCmdListRecorder::InitPSO() noexcept {
	ASSERT(sPSO == nullptr);
	ASSERT(sRootSign == nullptr);

	// Build pso and root signature
	PSOCreator::PSOParams psoParams{};
	const std::size_t rtCount{ _countof(psoParams.mRtFormats) };
	psoParams.mBlendDesc = D3DFactory::AlwaysBlendDesc();
	psoParams.mDepthStencilDesc = D3DFactory::DisableDepthStencilDesc();
	psoParams.mInputLayout = D3DFactory::PosNormalTangentTexCoordInputLayout();
	psoParams.mPSFilename = "EnvironmentLightPass/Shaders/PS.cso";
	psoParams.mRootSignFilename = "EnvironmentLightPass/Shaders/RS.cso";
	psoParams.mVSFilename = "EnvironmentLightPass/Shaders/VS.cso";
	psoParams.mNumRenderTargets = 1U;
	psoParams.mRtFormats[0U] = Settings::sColorBufferFormat;
	for (std::size_t i = psoParams.mNumRenderTargets; i < rtCount; ++i) {
		psoParams.mRtFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	psoParams.mTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PSOCreator::CreatePSO(psoParams, sPSO, sRootSign);

	ASSERT(sPSO != nullptr);
	ASSERT(sRootSign != nullptr);
}

void EnvironmentLightCmdListRecorder::Init(
	const BufferCreator::VertexBufferData& vertexBufferData,
	const BufferCreator::IndexBufferData indexBufferData,
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	ID3D12Resource& depthBuffer,
	ID3D12Resource& diffuseIrradianceCubeMap,
	ID3D12Resource& specularPreConvolvedCubeMap) noexcept
{
	ASSERT(ValidateData() == false);
	ASSERT(geometryBuffers != nullptr);
	ASSERT(geometryBuffersCount > 0U);

	mVertexBufferData = vertexBufferData;
	mIndexBufferData = indexBufferData;

	BuildBuffers(geometryBuffers, geometryBuffersCount, depthBuffer, diffuseIrradianceCubeMap, specularPreConvolvedCubeMap);

	ASSERT(ValidateData());
}

void EnvironmentLightCmdListRecorder::RecordCommandLists(
	const FrameCBuffer& frameCBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& rtvCpuDescHandle,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept {

	ASSERT(ValidateData());
	ASSERT(sPSO != nullptr);
	ASSERT(sRootSign != nullptr);

	ID3D12CommandAllocator* cmdAlloc{ mCmdAlloc[mCurrFrameIndex] };
	ASSERT(cmdAlloc != nullptr);

	// Update frame constants
	UploadBuffer& uploadFrameCBuffer(*mFrameCBuffer[mCurrFrameIndex]);
	uploadFrameCBuffer.CopyData(0U, &frameCBuffer, sizeof(frameCBuffer));
	
	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCmdList->Reset(cmdAlloc, sPSO));

	mCmdList->RSSetViewports(1U, &Settings::sScreenViewport);
	mCmdList->RSSetScissorRects(1U, &Settings::sScissorRect);
	mCmdList->OMSetRenderTargets(1U, &rtvCpuDescHandle, false, &depthStencilHandle);

	mCmdList->SetDescriptorHeaps(1U, &mCbvSrvUavDescHeap);
	mCmdList->SetGraphicsRootSignature(sRootSign);
	
	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set root parameters
	const D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(uploadFrameCBuffer.Resource()->GetGPUVirtualAddress());
	mCmdList->SetGraphicsRootConstantBufferView(0U, frameCBufferGpuVAddress);
	mCmdList->SetGraphicsRootConstantBufferView(1U, frameCBufferGpuVAddress);
	const D3D12_GPU_VIRTUAL_ADDRESS immutableBufferGpuVAddress(mImmutableCBuffer->Resource()->GetGPUVirtualAddress());
	mCmdList->SetGraphicsRootConstantBufferView(2U, immutableBufferGpuVAddress);
	mCmdList->SetGraphicsRootDescriptorTable(3U, mCbvSrvUavDescHeap->GetGPUDescriptorHandleForHeapStart());

	// Draw object
	mCmdList->IASetVertexBuffers(0U, 1U, &mVertexBufferData.mBufferView);
	mCmdList->IASetIndexBuffer(&mIndexBufferData.mBufferView);
	mCmdList->DrawIndexedInstanced(mIndexBufferData.mCount, 1U, 0U, 0U, 0U);

	mCmdList->Close();

	mCmdListQueue.push(mCmdList);

	// Next frame
	mCurrFrameIndex = (mCurrFrameIndex + 1) % Settings::sQueuedFrameCount;
}

bool EnvironmentLightCmdListRecorder::ValidateData() const noexcept {

	for (std::uint32_t i = 0UL; i < Settings::sQueuedFrameCount; ++i) {
		if (mCmdAlloc[i] == nullptr) {
			return false;
		}
	}

	const bool result =
		mCmdList != nullptr &&
		mCbvSrvUavDescHeap != nullptr &&
		mImmutableCBuffer != nullptr &&
		mFrameCBuffer != nullptr && 
		mCubeMapsBufferGpuDescHandleBegin.ptr != 0UL;

	return result;
}

void EnvironmentLightCmdListRecorder::BuildBuffers(
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers, 
	const std::uint32_t geometryBuffersCount,
	ID3D12Resource& depthBuffer,
	ID3D12Resource& diffuseIrradianceCubeMap,
	ID3D12Resource& specularPreConvolvedCubeMap) noexcept
{
	ASSERT(mCbvSrvUavDescHeap == nullptr);
	ASSERT(geometryBuffers != nullptr);
	ASSERT(geometryBuffersCount > 0U);

	// Create CBV_SRV_UAV cbuffer descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0U;
	descHeapDesc.NumDescriptors = geometryBuffersCount + 3U; // Geometry buffers + 1 depth buffer + 2 cube maps (diffuse + specular)
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ResourceManager::Get().CreateDescriptorHeap(descHeapDesc, mCbvSrvUavDescHeap);
	
	// Create geometry buffer texture descriptors
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	const std::size_t descHandleIncSize{ mDevice.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	D3D12_CPU_DESCRIPTOR_HANDLE cpuDesc = mCbvSrvUavDescHeap->GetCPUDescriptorHandleForHeapStart();
	for (std::uint32_t i = 0U; i < geometryBuffersCount; ++i) {
		ASSERT(geometryBuffers[i].Get() != nullptr);
		ID3D12Resource& res = *geometryBuffers[i].Get();
		srvDesc.Format = res.GetDesc().Format;
		srvDesc.Texture2D.MipLevels = res.GetDesc().MipLevels;
		ResourceManager::Get().CreateShaderResourceView(res, srvDesc, cpuDesc);
		cpuDesc.ptr += descHandleIncSize;
	}

	// Create depth buffer descriptor
	srvDesc.Format = Settings::sDepthStencilSRVFormat;
	srvDesc.Texture2D.MipLevels = depthBuffer.GetDesc().MipLevels;
	ResourceManager::Get().CreateShaderResourceView(depthBuffer, srvDesc, cpuDesc);

	// Create immutable cbuffer
	const std::size_t immutableCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(ImmutableCBuffer)) };
	ResourceManager::Get().CreateUploadBuffer(immutableCBufferElemSize, 1U, mImmutableCBuffer);
	ImmutableCBuffer immutableCBuffer;
	mImmutableCBuffer->CopyData(0U, &immutableCBuffer, sizeof(immutableCBuffer));

	// Create frame cbuffers
	const std::size_t frameCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(FrameCBuffer)) };
	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		ResourceManager::Get().CreateUploadBuffer(frameCBufferElemSize, 1U, mFrameCBuffer[i]);
	}

	// Create cube map texture descriptors

	mCubeMapsBufferGpuDescHandleBegin.ptr = mCbvSrvUavDescHeap->GetGPUDescriptorHandleForHeapStart().ptr + geometryBuffersCount;
	
	srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = diffuseIrradianceCubeMap.GetDesc().MipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = diffuseIrradianceCubeMap.GetDesc().Format;
	cpuDesc.ptr += descHandleIncSize;
	ResourceManager::Get().CreateShaderResourceView(diffuseIrradianceCubeMap, srvDesc, cpuDesc);

	srvDesc.TextureCube.MipLevels = specularPreConvolvedCubeMap.GetDesc().MipLevels;
	srvDesc.Format = specularPreConvolvedCubeMap.GetDesc().Format;
	cpuDesc.ptr += descHandleIncSize;
	ResourceManager::Get().CreateShaderResourceView(specularPreConvolvedCubeMap, srvDesc, cpuDesc);
}
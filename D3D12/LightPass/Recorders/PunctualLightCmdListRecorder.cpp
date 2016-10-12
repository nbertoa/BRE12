#include "PunctualLightCmdListRecorder.h"

#include <DirectXMath.h>

#include <LightPass/PunctualLight.h>
#include <MathUtils/MathUtils.h>
#include <PSOCreator/PSOCreator.h>
#include <ResourceManager/ResourceManager.h>
#include <ResourceManager/UploadBuffer.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils/DebugUtils.h>

namespace {
	ID3D12PipelineState* sPSO{ nullptr };
	ID3D12RootSignature* sRootSign{ nullptr };

	void CreateGeometryBuffersSRVs(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		D3D12_CPU_DESCRIPTOR_HANDLE baseCpuDescHandle) {
		ASSERT(geometryBuffers != nullptr);
		ASSERT(baseCpuDescHandle.ptr != 0UL);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		const std::size_t descHandleIncSize{ ResourceManager::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
		for (std::uint32_t i = 0U; i < geometryBuffersCount; ++i) {
			ID3D12Resource& res{ *geometryBuffers[i].Get() };

			srvDesc.Format = res.GetDesc().Format;
			srvDesc.Texture2D.MipLevels = res.GetDesc().MipLevels;

			ResourceManager::Get().CreateShaderResourceView(res, srvDesc, baseCpuDescHandle);

			baseCpuDescHandle.ptr += descHandleIncSize;
		}
	}

	void CreateLightsBufferSRV(ID3D12Resource& res, const std::uint32_t numLights, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescHandle) {
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = res.GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0UL;
		srvDesc.Buffer.NumElements = numLights;
		srvDesc.Buffer.StructureByteStride = sizeof(PunctualLight);
		ResourceManager::Get().CreateShaderResourceView(res, srvDesc, cpuDescHandle);
	}
}

PunctualLightCmdListRecorder::PunctualLightCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue)
	: LightPassCmdListRecorder(device, cmdListQueue)
{
}

void PunctualLightCmdListRecorder::InitPSO() noexcept {
	ASSERT(sPSO == nullptr);
	ASSERT(sRootSign == nullptr);

	// Build pso and root signature
	PSOCreator::PSOParams psoParams{};
	const std::size_t rtCount{ _countof(psoParams.mRtFormats) };
	psoParams.mBlendDesc = D3DFactory::AlwaysBlendDesc();
	psoParams.mDepthStencilDesc = D3DFactory::DisableDepthStencilDesc();
	psoParams.mGSFilename = "LightPass/Shaders/PunctualLight/GS.cso";
	psoParams.mPSFilename = "LightPass/Shaders/PunctualLight/PS.cso";
	psoParams.mRootSignFilename = "LightPass/Shaders/PunctualLight/RS.cso";
	psoParams.mVSFilename = "LightPass/Shaders/PunctualLight/VS.cso";
	psoParams.mNumRenderTargets = 1U;
	psoParams.mRtFormats[0U] = Settings::sColorBufferFormat;
	for (std::size_t i = psoParams.mNumRenderTargets; i < rtCount; ++i) {
		psoParams.mRtFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	psoParams.mTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	PSOCreator::CreatePSO(psoParams, sPSO, sRootSign);

	ASSERT(sPSO != nullptr);
	ASSERT(sRootSign != nullptr);
}

void PunctualLightCmdListRecorder::Init(
	Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
	const std::uint32_t geometryBuffersCount,
	const PunctualLight* lights,
	const std::uint32_t numLights) noexcept
{
	ASSERT(ValidateData() == false);
	ASSERT(geometryBuffers != nullptr);
	ASSERT(0 < geometryBuffersCount && geometryBuffersCount < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);
	ASSERT(lights != nullptr);
	ASSERT(numLights > 0U);

	mNumLights = numLights;

	// Geometry buffers SRVS + lights buffer SRV + cube map SRV
	const std::uint32_t descHeapOffset(geometryBuffersCount + 2U);
	BuildBuffers(lights, descHeapOffset);

	// Create geometry buffers SRVs
	D3D12_CPU_DESCRIPTOR_HANDLE cbvSrvUavCpuDescHandle(mCbvSrvUavDescHeap->GetCPUDescriptorHandleForHeapStart());
	CreateGeometryBuffersSRVs(geometryBuffers, geometryBuffersCount, cbvSrvUavCpuDescHandle);

	// Create lights buffer SRV	
	const std::size_t descHandleIncSize{ ResourceManager::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	D3D12_CPU_DESCRIPTOR_HANDLE lightsBufferCpuDescHandle{ cbvSrvUavCpuDescHandle.ptr + (geometryBuffersCount + 1U) * descHandleIncSize };
	CreateLightsBufferSRV(*mLightsBuffer->Resource(), mNumLights, lightsBufferCpuDescHandle);
	mLightsBufferGpuDescHandleBegin.ptr = mCbvSrvUavDescHeap->GetGPUDescriptorHandleForHeapStart().ptr + (geometryBuffersCount + 1U) * descHandleIncSize;

	ASSERT(ValidateData());

}

void PunctualLightCmdListRecorder::RecordCommandLists(
	const FrameCBuffer& frameCBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE* rtvCpuDescHandles,
	const std::uint32_t rtvCpuDescHandlesCount,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept {

	ASSERT(ValidateData());
	ASSERT(rtvCpuDescHandles != nullptr);
	ASSERT(rtvCpuDescHandlesCount > 0);
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
	mCmdList->OMSetRenderTargets(rtvCpuDescHandlesCount, rtvCpuDescHandles, false, &depthStencilHandle);

	mCmdList->SetDescriptorHeaps(1U, &mCbvSrvUavDescHeap);
	mCmdList->SetGraphicsRootSignature(sRootSign);

	// Set root parameters
	const D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(uploadFrameCBuffer.Resource()->GetGPUVirtualAddress());
	const D3D12_GPU_VIRTUAL_ADDRESS immutableCBufferGpuVAddress(mImmutableCBuffer->Resource()->GetGPUVirtualAddress());
	mCmdList->SetGraphicsRootConstantBufferView(0U, frameCBufferGpuVAddress);
	mCmdList->SetGraphicsRootDescriptorTable(1U, mLightsBufferGpuDescHandleBegin);
	mCmdList->SetGraphicsRootConstantBufferView(2U, frameCBufferGpuVAddress);
	mCmdList->SetGraphicsRootConstantBufferView(3U, immutableCBufferGpuVAddress);
	mCmdList->SetGraphicsRootConstantBufferView(4U, immutableCBufferGpuVAddress);
	mCmdList->SetGraphicsRootDescriptorTable(5U, mCbvSrvUavDescHeap->GetGPUDescriptorHandleForHeapStart());
	
	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	
	mCmdList->DrawInstanced(mNumLights, 1U, 0U, 0U);

	mCmdList->Close();

	mCmdListQueue.push(mCmdList);

	// Next frame
	mCurrFrameIndex = (mCurrFrameIndex + 1) % _countof(mCmdAlloc);
}

void PunctualLightCmdListRecorder::BuildBuffers(const PunctualLight* lights, const std::uint32_t descHeapOffset) noexcept {
	ASSERT(mCbvSrvUavDescHeap == nullptr);
#ifdef _DEBUG
	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		ASSERT(mFrameCBuffer[i] == nullptr);
	}
#endif
	ASSERT(mLightsBuffer == nullptr);
	ASSERT(lights != nullptr);

	// We assume we have world matrices by geometry index 0 (but we do not have geometry here)
	ASSERT(mNumLights != 0U);

	// Create CBV_SRV_UAV cbuffer descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0U;
	descHeapDesc.NumDescriptors = descHeapOffset + mNumLights;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ResourceManager::Get().CreateDescriptorHeap(descHeapDesc, mCbvSrvUavDescHeap);

	// Create lights buffer and fill it
	const std::size_t lightBufferElemSize{ sizeof(PunctualLight) };
	ResourceManager::Get().CreateUploadBuffer(lightBufferElemSize, mNumLights, mLightsBuffer);
	for (std::uint32_t i = 0UL; i < mNumLights; ++i) {
		mLightsBuffer->CopyData(i, &lights[i], sizeof(PunctualLight));
	}

	// Create frame cbuffers
	const std::size_t frameCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(FrameCBuffer)) };
	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		ResourceManager::Get().CreateUploadBuffer(frameCBufferElemSize, 1U, mFrameCBuffer[i]);
	}

	// Create immutable cbuffer
	const std::size_t immutableCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(ImmutableCBuffer)) };
	ResourceManager::Get().CreateUploadBuffer(immutableCBufferElemSize, 1U, mImmutableCBuffer);
	ImmutableCBuffer immutableCBuffer;
	mImmutableCBuffer->CopyData(0U, &immutableCBuffer, sizeof(immutableCBuffer));
}
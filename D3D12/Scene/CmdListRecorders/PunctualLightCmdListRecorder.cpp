#include "PunctualLightCmdListRecorder.h"

#include <DirectXMath.h>

#include <DXUtils/PunctualLight.h>
#include <MathUtils/MathUtils.h>
#include <PSOCreator/PSOCreator.h>
#include <ResourceManager/ResourceManager.h>
#include <ResourceManager/UploadBuffer.h>
#include <Utils/DebugUtils.h>

namespace {
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
		srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{};
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
	: CmdListRecorder(device, cmdListQueue)
{
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

	const PSOCreator::PSOData& psoData(PSOCreator::CommonPSOData::GetData(PSOCreator::CommonPSOData::PUNCTUAL_LIGHT));
	mPSO = psoData.mPSO;
	mRootSign = psoData.mRootSign;
	mNumLights = numLights;

	const std::uint32_t descHeapOffset(geometryBuffersCount + 1U);
	BuildBuffers(lights, descHeapOffset);

	// Create geometry buffers SRVs
	D3D12_CPU_DESCRIPTOR_HANDLE cbvSrvUavCpuDescHandle(mCbvSrvUavDescHeap->GetCPUDescriptorHandleForHeapStart());
	CreateGeometryBuffersSRVs(geometryBuffers, geometryBuffersCount, cbvSrvUavCpuDescHandle);

	// Create lights buffer SRV
	const std::size_t descHandleIncSize{ ResourceManager::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	D3D12_CPU_DESCRIPTOR_HANDLE lightsBufferCpuDescHandle{ cbvSrvUavCpuDescHandle.ptr + geometryBuffersCount * descHandleIncSize };
	CreateLightsBufferSRV(*mLightsBuffer->Resource(), mNumLights, lightsBufferCpuDescHandle);
	mLightsBufferGpuDescHandleBegin.ptr = mCbvSrvUavDescHeap->GetGPUDescriptorHandleForHeapStart().ptr + geometryBuffersCount * descHandleIncSize;

	ASSERT(ValidateData());

}

void PunctualLightCmdListRecorder::RecordCommandLists(
	const DirectX::XMFLOAT4X4& view,
	const DirectX::XMFLOAT4X4& proj,
	const D3D12_CPU_DESCRIPTOR_HANDLE* rtvCpuDescHandles,
	const std::uint32_t rtvCpuDescHandlesCount,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept {
	ASSERT(ValidateData());
	ASSERT(rtvCpuDescHandles != nullptr);
	ASSERT(rtvCpuDescHandlesCount > 0);

	ID3D12CommandAllocator* cmdAlloc{ mCmdAlloc[mCurrFrameIndex] };
	ASSERT(cmdAlloc != nullptr);	

	// Update frame constants
	DirectX::XMFLOAT4X4 matrices[2U];
	DirectX::XMStoreFloat4x4(&matrices[0], MathUtils::GetTranspose(view));
	DirectX::XMStoreFloat4x4(&matrices[1], MathUtils::GetTranspose(proj));
	UploadBuffer& frameCBuffer{ *mFrameCBuffer[mCurrFrameIndex] };
	frameCBuffer.CopyData(0U, &matrices, sizeof(matrices));

	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCmdList->Reset(cmdAlloc, mPSO));

	mCmdList->RSSetViewports(1U, &mScreenViewport);
	mCmdList->RSSetScissorRects(1U, &mScissorRect);
	mCmdList->OMSetRenderTargets(rtvCpuDescHandlesCount, rtvCpuDescHandles, false, &depthStencilHandle);

	mCmdList->SetDescriptorHeaps(1U, &mCbvSrvUavDescHeap);
	mCmdList->SetGraphicsRootSignature(mRootSign);

	// Set root parameters
	const D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(frameCBuffer.Resource()->GetGPUVirtualAddress());
	mCmdList->SetGraphicsRootConstantBufferView(0U, frameCBufferGpuVAddress);
	mCmdList->SetGraphicsRootDescriptorTable(1U, mLightsBufferGpuDescHandleBegin);
	mCmdList->SetGraphicsRootConstantBufferView(2U, frameCBufferGpuVAddress);
	mCmdList->SetGraphicsRootDescriptorTable(3U, mCbvSrvUavDescHeap->GetGPUDescriptorHandleForHeapStart());
	
	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	
	mCmdList->DrawInstanced(mNumLights, 1U, 0U, 0U);

	mCmdList->Close();

	mCmdListQueue.push(mCmdList);

	// Next frame
	mCurrFrameIndex = (mCurrFrameIndex + 1) % _countof(mCmdAlloc);
}

bool PunctualLightCmdListRecorder::ValidateData() const noexcept {
	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		if (mFrameCBuffer[i] == nullptr) {
			return false;
		}
	}

	const bool result =
		CmdListRecorder::ValidateData() &&
		mNumLights != 0U &&
		mLightsBuffer != nullptr &&
		mLightsBufferGpuDescHandleBegin.ptr != 0UL;

	return result;
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
	const std::size_t frameCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4) * 2UL) };
	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		ResourceManager::Get().CreateUploadBuffer(frameCBufferElemSize, 1U, mFrameCBuffer[i]);
	}
}
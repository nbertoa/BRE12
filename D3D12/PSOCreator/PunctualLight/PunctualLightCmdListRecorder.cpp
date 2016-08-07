#include "PunctualLightCmdListRecorder.h"

#include <DirectXMath.h>

#include <MathUtils/MathHelper.h>
#include <ResourceManager/ResourceManager.h>
#include <ResourceManager/UploadBuffer.h>
#include <Utils/DebugUtils.h>

PunctualLightCmdListRecorder::PunctualLightCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue)
	: CmdListRecorder(device, cmdListQueue)
{
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

	ID3D12CommandAllocator* cmdAlloc{ mCmdAlloc[mCurrCmdAllocIndex] };
	ASSERT(cmdAlloc != nullptr);
	mCurrCmdAllocIndex = (mCurrCmdAllocIndex + 1) % _countof(mCmdAlloc);

	// Update frame constants
	DirectX::XMFLOAT4X4 vp[2U];
	DirectX::XMStoreFloat4x4(&vp[0], MathHelper::GetTranspose(view));
	DirectX::XMStoreFloat4x4(&vp[1], MathHelper::GetTranspose(proj));
	mFrameCBuffer->CopyData(0U, &vp, sizeof(vp));

	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCmdList->Reset(cmdAlloc, mPSO));

	mCmdList->RSSetViewports(1U, &mScreenViewport);
	mCmdList->RSSetScissorRects(1U, &mScissorRect);
	mCmdList->OMSetRenderTargets(rtvCpuDescHandlesCount, rtvCpuDescHandles, false, &depthStencilHandle);

	mCmdList->SetDescriptorHeaps(1U, &mCbvSrvUavDescHeap);
	mCmdList->SetGraphicsRootSignature(mRootSign);

	// Set frame constants root parameters
	mCmdList->SetGraphicsRootConstantBufferView(1U, mFrameCBuffer->Resource()->GetGPUVirtualAddress());
	mCmdList->SetGraphicsRootConstantBufferView(2U, mFrameCBuffer->Resource()->GetGPUVirtualAddress());
	mCmdList->SetGraphicsRootDescriptorTable(3U, CbvSrvUavDescHeap()->GetGPUDescriptorHandleForHeapStart());

	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

	const std::size_t descHandleIncSize{ mDevice.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	D3D12_GPU_DESCRIPTOR_HANDLE objectCBufferGpuDescHandle(mObjectCBufferGpuDescHandleBegin);
	
	// Draw objects
	const std::size_t worldMatsCount{ mWorldMatricesByGeomIndex[0].size() };
	for (std::size_t j = 0UL; j < worldMatsCount; ++j) {
		mCmdList->SetGraphicsRootDescriptorTable(0U, objectCBufferGpuDescHandle);
		objectCBufferGpuDescHandle.ptr += descHandleIncSize;

		mCmdList->DrawInstanced(1U, 1U, 0U, 0U);
	}

	mCmdList->Close();

	mCmdListQueue.push(mCmdList);
}

bool PunctualLightCmdListRecorder::ValidateData() const noexcept {
	return CmdListRecorder::ValidateData() && mFrameCBuffer != nullptr && mWorldMatricesByGeomIndex.size() == 1UL && mWorldMatricesByGeomIndex[0].empty() == false;
}
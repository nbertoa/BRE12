#include "BasicTechCmdBuilderTask.h"

#include <DirectXMath.h>

#include <MathUtils/MathHelper.h>
#include <ResourceManager/ResourceManager.h>
#include <ResourceManager/UploadBuffer.h>
#include <Utils/DebugUtils.h>

BasicTechCmdBuilderTask::BasicTechCmdBuilderTask(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue)
	: CmdListRecorder(device, cmdListQueue)
{
}

void BasicTechCmdBuilderTask::RecordCommandLists(
	const DirectX::XMFLOAT4X4& view,
	const DirectX::XMFLOAT4X4& proj,
	const D3D12_CPU_DESCRIPTOR_HANDLE& backBufferHandle,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept {
	ASSERT(ValidateData());

	ID3D12CommandAllocator* cmdAlloc{ mCmdAlloc[mCurrCmdAllocIndex] };
	ASSERT(cmdAlloc != nullptr);
	mCurrCmdAllocIndex = (mCurrCmdAllocIndex + 1) % _countof(mCmdAlloc);

	// Update frame constants
	DirectX::XMFLOAT4X4 vp[2U];
	DirectX::XMStoreFloat4x4(&vp[0], MathHelper::GetTranspose(view));
	DirectX::XMStoreFloat4x4(&vp[1], MathHelper::GetTranspose(proj));
	mFrameConstants->CopyData(0U, &vp, sizeof(vp));

	// Update object constants
	const std::size_t geomCount{ mGeometryVec.size() };
	for (std::size_t i = 0UL; i < geomCount; ++i) {
		const std::uint32_t worldMatsCount{ (std::uint32_t)mWorldMatricesByGeomIndex[i].size() };
		for (std::uint32_t j = 0UL; j < worldMatsCount; ++j) {
			DirectX::XMFLOAT4X4 w;
			const DirectX::XMMATRIX wMatrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&mWorldMatricesByGeomIndex[i][j]));
			DirectX::XMStoreFloat4x4(&w, wMatrix);
			mObjectConstants->CopyData(j, &w, sizeof(w));
		}
	}

	// We do not update materials constant buffer because it was filled at creation and
	// it is immutable.
	
	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCmdList->Reset(cmdAlloc, mPSO));

	mCmdList->RSSetViewports(1U, &mScreenViewport);
	mCmdList->RSSetScissorRects(1U, &mScissorRect);
	mCmdList->OMSetRenderTargets(1U, &backBufferHandle, true, &depthStencilHandle);

	mCmdList->SetDescriptorHeaps(1U, &mCBVHeap);
	mCmdList->SetGraphicsRootSignature(mRootSign);

	const std::size_t descHandleIncSize{ mDevice.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	D3D12_GPU_DESCRIPTOR_HANDLE cbvHeapGPUDescHandle = mCBVHeap->GetGPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE materialsHeapGpuDescHandle(mMaterialsGpuDescHandleBegin);
		
	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set objects constants root parameter
	for (std::size_t i = 0UL; i < geomCount; ++i) {
		mCmdList->IASetVertexBuffers(0U, 1U, &mGeometryVec[i].mVertexBufferView);
		mCmdList->IASetIndexBuffer(&mGeometryVec[i].mIndexBufferView);
		const std::size_t worldMatsCount{ mWorldMatricesByGeomIndex[i].size() };
		for (std::size_t j = 0UL; j < worldMatsCount; ++j) {
			mCmdList->SetGraphicsRootDescriptorTable(0U, cbvHeapGPUDescHandle);
			cbvHeapGPUDescHandle.ptr += descHandleIncSize;
			mCmdList->SetGraphicsRootDescriptorTable(2U, materialsHeapGpuDescHandle);
			materialsHeapGpuDescHandle.ptr += descHandleIncSize;
			mCmdList->DrawIndexedInstanced(mGeometryVec[i].mIndexCount, 1U, 0U, 0U, 0U);
		}
	}

	// Set frame constants root parameter
	mCmdList->SetGraphicsRootConstantBufferView(1U, mFrameConstants->Resource()->GetGPUVirtualAddress());
	mCmdList->SetGraphicsRootConstantBufferView(3U, mFrameConstants->Resource()->GetGPUVirtualAddress());
	
	mCmdList->Close();

	mCmdListQueue.push(mCmdList);
}

bool BasicTechCmdBuilderTask::ValidateData() const noexcept {
	return CmdListRecorder::ValidateData() && mMaterialsBuffer != nullptr;
}
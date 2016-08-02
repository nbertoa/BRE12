#include "BasicCmdListRecorder.h"

#include <DirectXMath.h>

#include <MathUtils/MathHelper.h>
#include <ResourceManager/ResourceManager.h>
#include <ResourceManager/UploadBuffer.h>
#include <Utils/DebugUtils.h>

BasicCmdListRecorder::BasicCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue)
	: CmdListRecorder(device, cmdListQueue)
{
}

void BasicCmdListRecorder::RecordCommandLists(
	const DirectX::XMFLOAT4X4& view,
	const DirectX::XMFLOAT4X4& proj,
	const D3D12_CPU_DESCRIPTOR_HANDLE* geomPassRtvCpuDescHandles,
	const std::uint32_t geomPassRtvCpuDescHandlesCount,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept {
	ASSERT(ValidateData());
	ASSERT(geomPassRtvCpuDescHandles != nullptr);
	ASSERT(geomPassRtvCpuDescHandlesCount > 0);

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
	mCmdList->OMSetRenderTargets(geomPassRtvCpuDescHandlesCount, geomPassRtvCpuDescHandles, false, &depthStencilHandle);

	mCmdList->SetDescriptorHeaps(1U, &mCbvSrvUavDescHeap);
	mCmdList->SetGraphicsRootSignature(mRootSign);

	const std::size_t descHandleIncSize{ mDevice.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	D3D12_GPU_DESCRIPTOR_HANDLE objectCBufferGpuDescHandle(mObjectCBufferGpuDescHandleBegin);
	D3D12_GPU_DESCRIPTOR_HANDLE materialsCBufferGpuDescHandle(mMaterialsCBufferGpuDescHandleBegin);
		
	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Draw objects
	const std::size_t geomCount{ mGeometryVec.size() };
	for (std::size_t i = 0UL; i < geomCount; ++i) {
		mCmdList->IASetVertexBuffers(0U, 1U, &mGeometryVec[i].mVertexBufferView);
		mCmdList->IASetIndexBuffer(&mGeometryVec[i].mIndexBufferView);
		const std::size_t worldMatsCount{ mWorldMatricesByGeomIndex[i].size() };
		for (std::size_t j = 0UL; j < worldMatsCount; ++j) {
			mCmdList->SetGraphicsRootDescriptorTable(0U, objectCBufferGpuDescHandle);
			objectCBufferGpuDescHandle.ptr += descHandleIncSize;
			mCmdList->SetGraphicsRootDescriptorTable(2U, materialsCBufferGpuDescHandle);
			materialsCBufferGpuDescHandle.ptr += descHandleIncSize;
			mCmdList->DrawIndexedInstanced(mGeometryVec[i].mIndexCount, 1U, 0U, 0U, 0U);
		}
	}

	// Set frame constants root parameters
	mCmdList->SetGraphicsRootConstantBufferView(1U, mFrameCBuffer->Resource()->GetGPUVirtualAddress());
	mCmdList->SetGraphicsRootConstantBufferView(3U, mFrameCBuffer->Resource()->GetGPUVirtualAddress());
	
	mCmdList->Close();

	mCmdListQueue.push(mCmdList);
}

bool BasicCmdListRecorder::ValidateData() const noexcept {
	return CmdListRecorder::ValidateData() && mFrameCBuffer != nullptr && mObjectCBuffer != nullptr && mGeometryVec.empty() == false && mMaterialsCBuffer != nullptr;
}
#include "ShapesCmdBuilderTask.h"

#include <DirectXMath.h>

#include <MathUtils/MathHelper.h>
#include <ResourceManager/ResourceManager.h>
#include <ResourceManager/UploadBuffer.h>
#include <Utils/DebugUtils.h>

ShapesCmdBuilderTask::ShapesCmdBuilderTask(ID3D12Device* device, const D3D12_VIEWPORT& screenViewport, const D3D12_RECT& scissorRect)
	: CmdBuilderTask(device, screenViewport, scissorRect)
{
	ASSERT(device != nullptr);
}

void ShapesCmdBuilderTask::BuildCommandLists(
	tbb::concurrent_queue<ID3D12CommandList*>& cmdLists,
	const DirectX::XMFLOAT4X4& view,
	const DirectX::XMFLOAT4X4& proj,
	const D3D12_CPU_DESCRIPTOR_HANDLE& backBufferHandle,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept {

	ID3D12CommandAllocator* cmdAlloc{ mInput.mCmdAlloc[mInput.mCurrCmdAllocIndex] };
	ASSERT(cmdAlloc != nullptr);
	mInput.mCurrCmdAllocIndex = (mInput.mCurrCmdAllocIndex + 1) % _countof(mInput.mCmdAlloc);

	// Update view projection matrix
	ASSERT(mInput.mFrameConstants != nullptr);
	DirectX::XMFLOAT4X4 vp;
	DirectX::XMStoreFloat4x4(&vp, MathHelper::GetTransposeViewProj(view, proj));
	mInput.mFrameConstants->CopyData(0U, &vp, sizeof(vp));

	// Update world
	ASSERT(mInput.mObjectConstants != nullptr);
	const std::size_t geomCount{ mInput.mGeomDataVec.size() };
	for (std::size_t i = 0UL; i < geomCount; ++i) {
		const GeometryData& geomData{ mInput.mGeomDataVec[i] };
		const std::uint32_t worldMatsCount{ (std::uint32_t)geomData.mWorldMats.size() };
		for (std::uint32_t j = 0UL; j < worldMatsCount; ++j) {
			DirectX::XMFLOAT4X4 w;
			const DirectX::XMMATRIX wMatrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&geomData.mWorldMats[j]));
			DirectX::XMStoreFloat4x4(&w, wMatrix);
			mInput.mObjectConstants->CopyData(j, &w, sizeof(w));
		}
	}

	ASSERT(mInput.mCBVHeap != nullptr);
	ASSERT(mInput.mRootSign != nullptr);
	ASSERT(mInput.mPSO != nullptr);

	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mInput.mCmdList->Reset(cmdAlloc, mInput.mPSO));

	mInput.mCmdList->RSSetViewports(1U, &mViewport);
	mInput.mCmdList->RSSetScissorRects(1U, &mScissorRect);
	mInput.mCmdList->OMSetRenderTargets(1U, &backBufferHandle, true, &depthStencilHandle);

	mInput.mCmdList->SetDescriptorHeaps(1U, &mInput.mCBVHeap);
	mInput.mCmdList->SetGraphicsRootSignature(mInput.mRootSign);
	D3D12_GPU_DESCRIPTOR_HANDLE cbvHeapGPUDescHandle = mInput.mCbvBaseGpuDescHandle;
	const std::size_t descHandleIncSize{ mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };	
	mInput.mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set objects constants root parameter
	for (std::size_t i = 0UL; i < geomCount; ++i) {
		const GeometryData& geomData{ mInput.mGeomDataVec[i] };
		mInput.mCmdList->IASetVertexBuffers(0U, 1U, &geomData.mBuffersInfo.mVertexBufferView);
		mInput.mCmdList->IASetIndexBuffer(&geomData.mBuffersInfo.mIndexBufferView);
		const std::size_t worldMatsCount{ geomData.mWorldMats.size() };
		for (std::size_t j = 0UL; j < worldMatsCount; ++j) {
			mInput.mCmdList->SetGraphicsRootDescriptorTable(0U, cbvHeapGPUDescHandle);
			mInput.mCmdList->DrawIndexedInstanced(geomData.mBuffersInfo.mIndexCount, 1U, 0U, 0U, 0U);
			cbvHeapGPUDescHandle.ptr += descHandleIncSize;
		}
	}

	// Set frame constants root parameter
	D3D12_GPU_VIRTUAL_ADDRESS cbFrameGPUBaseAddress{ mInput.mFrameConstants->Resource()->GetGPUVirtualAddress() };
	mInput.mCmdList->SetGraphicsRootConstantBufferView(1U, cbFrameGPUBaseAddress);

	mInput.mCmdList->Close();

	cmdLists.push(mInput.mCmdList);
}
#include "ShapeTask.h"

#include <DirectXMath.h>

#include <Camera/Camera.h>
#include <ResourceManager/ResourceManager.h>
#include <ResourceManager/UploadBuffer.h>
#include <Utils/DebugUtils.h>

ShapeTask::ShapeTask(ID3D12Device* device, const D3D12_VIEWPORT& screenViewport, const D3D12_RECT& scissorRect)
	: CmdBuilderTask(device, screenViewport, scissorRect)
{
	ASSERT(device != nullptr);
}

void ShapeTask::Execute(
	tbb::concurrent_queue<ID3D12CommandList*>& cmdLists,
	const std::uint32_t currBackBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& backBufferHandle,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept {

	ID3D12CommandAllocator* cmdAlloc{ mInput.mCmdAlloc[currBackBuffer] };
	ASSERT(cmdAlloc != nullptr);

	// Update view projection matrix
	ASSERT(mInput.mFrameConstants != nullptr);
	DirectX::XMFLOAT4X4 vp;
	DirectX::XMStoreFloat4x4(&vp, Camera::gCamera->GetTransposeViewProj());
	mInput.mFrameConstants->CopyData(0U, &vp, sizeof(vp));

	// Update world
	ASSERT(mInput.mObjectConstants != nullptr);
	const std::uint32_t geomCount{ (std::uint32_t)mInput.mGeomDataVec.size() };
	for (std::uint32_t i = 0U; i < geomCount; ++i) {
		const GeometryData& geomData{ mInput.mGeomDataVec[i] };
		DirectX::XMFLOAT4X4 w;
		const DirectX::XMMATRIX wMatrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&geomData.mWorld));
		DirectX::XMStoreFloat4x4(&w, wMatrix);
		mInput.mObjectConstants->CopyData(i, &w, sizeof(w));
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
	D3D12_GPU_DESCRIPTOR_HANDLE cbvHeapGPUDescHandle = mInput.mCBVHeap->GetGPUDescriptorHandleForHeapStart();
	const std::size_t descHandleIncSize{ mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };	
	mInput.mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set objects constants root parameter
	for (std::size_t i = 0UL; i < geomCount; ++i) {
		const GeometryData& geomData{ mInput.mGeomDataVec[i] };
		mInput.mCmdList->SetGraphicsRootDescriptorTable(0U, cbvHeapGPUDescHandle);		
		mInput.mCmdList->IASetVertexBuffers(0U, 1U, &geomData.mVertexBufferView);
		mInput.mCmdList->IASetIndexBuffer(&geomData.mIndexBufferView);
		mInput.mCmdList->DrawIndexedInstanced(geomData.mIndexCount, 1U, geomData.mStartIndexLoc, geomData.mBaseVertexLoc, 0U);
		cbvHeapGPUDescHandle.ptr += descHandleIncSize;
	}

	// Set frame constants root parameter
	D3D12_GPU_VIRTUAL_ADDRESS cbFrameGPUBaseAddress{ mInput.mFrameConstants->Resource()->GetGPUVirtualAddress() };
	mInput.mCmdList->SetGraphicsRootConstantBufferView(1U, cbFrameGPUBaseAddress);

	mInput.mCmdList->Close();

	cmdLists.push(mInput.mCmdList);
}
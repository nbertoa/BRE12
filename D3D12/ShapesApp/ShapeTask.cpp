#include "ShapeTask.h"

#include <DirectXMath.h>

#include <Camera/Camera.h>
#include <ResourceManager/ResourceManager.h>
#include <ResourceManager/UploadBuffer.h>
#include <Utils/DebugUtils.h>

namespace {
	struct Vertex {
		Vertex() {}
		Vertex(const DirectX::XMFLOAT4& p)
			: mPosition(p)
		{}

		DirectX::XMFLOAT4 mPosition{ 0.0f, 0.0f, 0.0f, 0.0f };
	};
}

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

	//SwitchCmdListAndAlloc();
	ID3D12CommandAllocator* cmdAlloc{ mInput.mCmdAlloc[currBackBuffer] };
	ASSERT(cmdAlloc != nullptr);

	// Update
	ASSERT(mInput.mObjectConstants != nullptr);
	const DirectX::XMMATRIX vpMatrix = DirectX::XMMatrixTranspose(Camera::gCamera->GetViewProj());
	const std::uint32_t geomCount{ (std::uint32_t)mInput.mGeomDataVec.size() };
	for (std::uint32_t i = 0U; i < geomCount; ++i) {
		const GeometryData& geomData{ mInput.mGeomDataVec[i] };
		DirectX::XMFLOAT4X4 wvpMatrix{};
		const DirectX::XMMATRIX wMatrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&geomData.mWorld));
		DirectX::XMStoreFloat4x4(&wvpMatrix, DirectX::XMMatrixMultiply(vpMatrix, wMatrix));
		mInput.mObjectConstants->CopyData(i, &wvpMatrix, sizeof(wvpMatrix));
	}

	ASSERT(mInput.mCBVHeap != nullptr);
	ASSERT(mInput.mRootSign != nullptr);
	ASSERT(mInput.mPSO != nullptr);

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	CHECK_HR(cmdAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	CHECK_HR(mInput.mCmdList->Reset(cmdAlloc, mInput.mPSO));

	mInput.mCmdList->RSSetViewports(1U, &mViewport);
	mInput.mCmdList->RSSetScissorRects(1U, &mScissorRect);
	mInput.mCmdList->OMSetRenderTargets(1U, &backBufferHandle, true, &depthStencilHandle);

	mInput.mCmdList->SetDescriptorHeaps(1U, &mInput.mCBVHeap);
	mInput.mCmdList->SetGraphicsRootSignature(mInput.mRootSign);
	D3D12_GPU_DESCRIPTOR_HANDLE cbvGpuDescHandle = mInput.mCBVHeap->GetGPUDescriptorHandleForHeapStart();
	const std::size_t descHandleIncSize{ mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };	
	mInput.mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (std::size_t i = 0UL; i < geomCount; ++i) {
		mInput.mCmdList->SetGraphicsRootDescriptorTable(0U, cbvGpuDescHandle);

		const GeometryData& geomData{ mInput.mGeomDataVec[i] };
		mInput.mCmdList->IASetVertexBuffers(0U, 1U, &geomData.mVertexBufferView);
		mInput.mCmdList->IASetIndexBuffer(&geomData.mIndexBufferView);
		mInput.mCmdList->DrawIndexedInstanced(geomData.mIndexCount, 1U, geomData.mStartIndexLoc, geomData.mBaseVertexLoc, 0U);
		cbvGpuDescHandle.ptr += descHandleIncSize;
	}

	mInput.mCmdList->Close();

	cmdLists.push(mInput.mCmdList);
}
#include "PunctualLightCmdListRecorder.h"

#include <DirectXMath.h>

#include <MathUtils/MathUtils.h>
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
	DirectX::XMStoreFloat4x4(&vp[0], MathUtils::GetTranspose(view));
	DirectX::XMStoreFloat4x4(&vp[1], MathUtils::GetTranspose(proj));
	mFrameCBuffer->CopyData(0U, &vp, sizeof(vp));

	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCmdList->Reset(cmdAlloc, mPSO));

	mCmdList->RSSetViewports(1U, &mScreenViewport);
	mCmdList->RSSetScissorRects(1U, &mScissorRect);
	mCmdList->OMSetRenderTargets(rtvCpuDescHandlesCount, rtvCpuDescHandles, false, &depthStencilHandle);

	mCmdList->SetDescriptorHeaps(1U, &mCbvSrvUavDescHeap);
	mCmdList->SetGraphicsRootSignature(mRootSign);

	// Set root parameters
	mCmdList->SetGraphicsRootConstantBufferView(0U, mFrameCBuffer->Resource()->GetGPUVirtualAddress());
	mCmdList->SetGraphicsRootConstantBufferView(1U, mLightCBuffer->Resource()->GetGPUVirtualAddress());//LightCBufferGpuDescHandleBegin());	
	mCmdList->SetGraphicsRootConstantBufferView(2U, mFrameCBuffer->Resource()->GetGPUVirtualAddress());
	mCmdList->SetGraphicsRootDescriptorTable(3U, CbvSrvUavDescHeap()->GetGPUDescriptorHandleForHeapStart());

	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	
	// Draw objects
	mCmdList->DrawInstanced(mNumLights, 1U, 0U, 0U);

	mCmdList->Close();

	mCmdListQueue.push(mCmdList);
}

bool PunctualLightCmdListRecorder::ValidateData() const noexcept {
	const bool result =  
		CmdListRecorder::ValidateData() &&
		mFrameCBuffer != nullptr &&
		mNumLights != 0U &&
		mNumLights <= sMaxNumLights &&
		mLightCBuffer != nullptr &&
		mLightCBufferGpuDescHandleBegin.ptr != 0UL;

	return result;
}
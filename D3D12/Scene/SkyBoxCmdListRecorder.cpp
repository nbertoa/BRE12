#include "SkyBoxCmdListRecorder.h"

#include <DirectXMath.h>

#include <CommandManager/CommandManager.h>
#include <DXUtils\CBuffers.h>
#include <PSOCreator/PSOCreator.h>
#include <ResourceManager/ResourceManager.h>
#include <ResourceManager/UploadBuffer.h>
#include <Utils/DebugUtils.h>

namespace {
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

SkyBoxCmdListRecorder::SkyBoxCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue)
	: mDevice(device)
	, mCmdListQueue(cmdListQueue)
{
	BuildCommandObjects(mCmdList, mCmdAlloc, _countof(mCmdAlloc));
}

void SkyBoxCmdListRecorder::Init(const GeometryData& geometryData, ID3D12Resource& cubeMap) noexcept
{
	ASSERT(ValidateData() == false);

	mGeometryData = geometryData;

	const PSOCreator::PSOData& psoData(PSOCreator::CommonPSOData::GetData(PSOCreator::CommonPSOData::SKY_BOX));

	mPSO = psoData.mPSO;
	mRootSign = psoData.mRootSign;

	BuildBuffers(cubeMap);

	ASSERT(ValidateData());
}

void SkyBoxCmdListRecorder::RecordCommandLists(
	const FrameCBuffer& frameCBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE* rtvCpuDescHandles,
	const std::uint32_t rtvCpuDescHandlesCount,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept {
	ASSERT(ValidateData());
	ASSERT(rtvCpuDescHandles != nullptr);
	ASSERT(rtvCpuDescHandlesCount > 0);

	ID3D12CommandAllocator* cmdAlloc{ mCmdAlloc[mCurrFrameIndex] };
	ASSERT(cmdAlloc != nullptr);

	// Update frame constants
	UploadBuffer& uploadFrameCBuffer(*mFrameCBuffer[mCurrFrameIndex]);
	uploadFrameCBuffer.CopyData(0U, &frameCBuffer, sizeof(frameCBuffer));

	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCmdList->Reset(cmdAlloc, mPSO));

	mCmdList->RSSetViewports(1U, &mScreenViewport);
	mCmdList->RSSetScissorRects(1U, &mScissorRect);
	mCmdList->OMSetRenderTargets(rtvCpuDescHandlesCount, rtvCpuDescHandles, false, &depthStencilHandle);

	mCmdList->SetDescriptorHeaps(1U, &mCbvSrvUavDescHeap);
	mCmdList->SetGraphicsRootSignature(mRootSign);

	const std::size_t descHandleIncSize{ mDevice.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	D3D12_GPU_DESCRIPTOR_HANDLE objectCBufferGpuDescHandle(mObjectCBufferGpuDescHandleBegin);
	D3D12_GPU_DESCRIPTOR_HANDLE cubeMapBufferGpuDescHandle(mCubeMapBufferGpuDescHandleBegin);

	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set frame constants root parameters
	D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(uploadFrameCBuffer.Resource()->GetGPUVirtualAddress());
	mCmdList->SetGraphicsRootConstantBufferView(1U, frameCBufferGpuVAddress);
	
	// Draw object
	mCmdList->IASetVertexBuffers(0U, 1U, &mGeometryData.mVertexBufferData.mBufferView);
	mCmdList->IASetIndexBuffer(&mGeometryData.mIndexBufferData.mBufferView);
	ASSERT(mGeometryData.mWorldMatrices.size() == 1UL);
	mCmdList->SetGraphicsRootDescriptorTable(0U, objectCBufferGpuDescHandle);
	mCmdList->SetGraphicsRootDescriptorTable(2U, cubeMapBufferGpuDescHandle);

	mCmdList->DrawIndexedInstanced(mGeometryData.mIndexBufferData.mCount, 1U, 0U, 0U, 0U);

	mCmdList->Close();

	mCmdListQueue.push(mCmdList);

	// Next frame
	mCurrFrameIndex = (mCurrFrameIndex + 1) % Settings::sQueuedFrameCount;
}

bool SkyBoxCmdListRecorder::ValidateData() const noexcept {

	for (std::uint32_t i = 0UL; i < Settings::sQueuedFrameCount; ++i) {
		if (mCmdAlloc[i] == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0UL; i < Settings::sQueuedFrameCount; ++i) {
		if (mFrameCBuffer[i] == nullptr) {
			return false;
		}
	}

	const bool result =
		mCmdList != nullptr &&
		mCbvSrvUavDescHeap != nullptr &&
		mTopology != D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED &&
		mRootSign != nullptr &&
		mPSO != nullptr &&
		mGeometryData.mWorldMatrices.size() == 1UL && 
		mObjectCBuffer != nullptr &&
		mObjectCBufferGpuDescHandleBegin.ptr != 0UL &&
		mCubeMapBufferGpuDescHandleBegin.ptr != 0UL;

	return result;
}

void SkyBoxCmdListRecorder::BuildBuffers(ID3D12Resource& cubeMap) noexcept {

	ASSERT(mCbvSrvUavDescHeap == nullptr);
#ifdef _DEBUG
	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		ASSERT(mFrameCBuffer[i] == nullptr);
	}
#endif
	ASSERT(mObjectCBuffer == nullptr);

	// Create CBV_SRV_UAV cbuffer descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0U;
	descHeapDesc.NumDescriptors = 2; // 1 obj cbuffer + 1 cube map texture
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ResourceManager::Get().CreateDescriptorHeap(descHeapDesc, mCbvSrvUavDescHeap);

	// Create object cbuffer and fill it
	const std::size_t objCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(ObjectCBuffer)) };
	ResourceManager::Get().CreateUploadBuffer(objCBufferElemSize, 1U, mObjectCBuffer);
	mObjectCBufferGpuDescHandleBegin = mCbvSrvUavDescHeap->GetGPUDescriptorHandleForHeapStart();
	ObjectCBuffer objCBuffer;
	ASSERT(mGeometryData.mWorldMatrices.size() == 1UL);
	const DirectX::XMMATRIX wMatrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&mGeometryData.mWorldMatrices[0]));
	DirectX::XMStoreFloat4x4(&objCBuffer.mWorld, wMatrix);
	mObjectCBuffer->CopyData(0U, &objCBuffer, sizeof(objCBuffer));
	
	// Set begin for cube map in GPU
	const std::size_t descHandleIncSize{ ResourceManager::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	mCubeMapBufferGpuDescHandleBegin.ptr = mObjectCBufferGpuDescHandleBegin.ptr + descHandleIncSize;

	// Create object cbuffer descriptor
	D3D12_CONSTANT_BUFFER_VIEW_DESC cBufferDesc{};
	const D3D12_GPU_VIRTUAL_ADDRESS objCBufferGpuAddress{ mObjectCBuffer->Resource()->GetGPUVirtualAddress() };
	cBufferDesc.BufferLocation = objCBufferGpuAddress;
	cBufferDesc.SizeInBytes = (std::uint32_t)objCBufferElemSize;
	const D3D12_CPU_DESCRIPTOR_HANDLE objCBufferDescHandle(mCbvSrvUavDescHeap->GetCPUDescriptorHandleForHeapStart());
	ResourceManager::Get().CreateConstantBufferView(cBufferDesc, objCBufferDescHandle);

	// Create cube map texture descriptor
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = cubeMap.GetDesc().MipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = cubeMap.GetDesc().Format;
	const D3D12_CPU_DESCRIPTOR_HANDLE cubeMapBufferDescHandle{ mCbvSrvUavDescHeap->GetCPUDescriptorHandleForHeapStart().ptr + descHandleIncSize };
	ResourceManager::Get().CreateShaderResourceView(cubeMap, srvDesc, cubeMapBufferDescHandle);

	// Create frame cbuffers
	const std::size_t frameCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(FrameCBuffer)) };
	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		ResourceManager::Get().CreateUploadBuffer(frameCBufferElemSize, 1U, mFrameCBuffer[i]);
	}
}
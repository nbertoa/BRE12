#include "BasicCmdListRecorder.h"

#include <DirectXMath.h>

#include <MathUtils/MathUtils.h>
#include <PSOCreator/Material.h>
#include <PSOCreator/PSOCreator.h>
#include <ResourceManager/ResourceManager.h>
#include <ResourceManager/UploadBuffer.h>
#include <Utils/DebugUtils.h>

BasicCmdListRecorder::BasicCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue)
	: CmdListRecorder(device, cmdListQueue)
{
}

void BasicCmdListRecorder::Init(
	const GeometryData* geometryDataVec,
	const std::uint32_t numGeomData,
	const Material* materials,
	const std::uint32_t numMaterials) noexcept
{
	ASSERT(ValidateData() == false);
	ASSERT(geometryDataVec != nullptr);
	ASSERT(numGeomData != 0U);
	ASSERT(materials != nullptr);
	ASSERT(numMaterials > 0UL);

	// Check that the total number of matrices (geometry to be drawn) will be equal to available materials
#ifdef _DEBUG
	std::size_t totalNumMatrices{ 0UL };
	for (std::size_t i = 0UL; i < numGeomData; ++i) {
		const std::size_t numMatrices{ geometryDataVec[i].mWorldMatrices.size() };
		totalNumMatrices += numMatrices;
		ASSERT(numMatrices != 0UL);
	}
	ASSERT(totalNumMatrices == numMaterials);
#endif
	mGeometryDataVec.reserve(numGeomData);
	for (std::uint32_t i = 0U; i < numGeomData; ++i) {
		mGeometryDataVec.push_back(geometryDataVec[i]);
	}

	const PSOCreator::PSOData& psoData(PSOCreator::CommonPSOData::GetData(PSOCreator::CommonPSOData::BASIC));

	mPSO = psoData.mPSO;
	mRootSign  = psoData.mRootSign;

	BuildBuffers(materials, numMaterials);

	ASSERT(ValidateData());
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

	ID3D12CommandAllocator* cmdAlloc{ mCmdAlloc[mCurrFrameIndex] };
	ASSERT(cmdAlloc != nullptr);	
	
	// Update frame constants
	DirectX::XMFLOAT4X4 vp[2U];
	DirectX::XMStoreFloat4x4(&vp[0], MathUtils::GetTranspose(view));
	DirectX::XMStoreFloat4x4(&vp[1], MathUtils::GetTranspose(proj));
	UploadBuffer& frameCBuffer(*mFrameCBuffer[mCurrFrameIndex]);
	frameCBuffer.CopyData(0U, &vp, sizeof(vp));

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

	// Set frame constants root parameters
	D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(frameCBuffer.Resource()->GetGPUVirtualAddress());
	mCmdList->SetGraphicsRootConstantBufferView(1U, frameCBufferGpuVAddress);
	mCmdList->SetGraphicsRootConstantBufferView(3U, frameCBufferGpuVAddress);

	// Draw objects
	const std::size_t geomCount{ mGeometryDataVec.size() };
	for (std::size_t i = 0UL; i < geomCount; ++i) {
		GeometryData& geomData{ mGeometryDataVec[i] };
		mCmdList->IASetVertexBuffers(0U, 1U, &geomData.mVertexBufferData.mBufferView);
		mCmdList->IASetIndexBuffer(&geomData.mIndexBufferData.mBufferView);
		const std::size_t worldMatsCount{ geomData.mWorldMatrices.size() };
		for (std::size_t j = 0UL; j < worldMatsCount; ++j) {
			mCmdList->SetGraphicsRootDescriptorTable(0U, objectCBufferGpuDescHandle);
			objectCBufferGpuDescHandle.ptr += descHandleIncSize;

			mCmdList->SetGraphicsRootDescriptorTable(2U, materialsCBufferGpuDescHandle);
			materialsCBufferGpuDescHandle.ptr += descHandleIncSize;

			mCmdList->DrawIndexedInstanced(geomData.mIndexBufferData.mCount, 1U, 0U, 0U, 0U);
		}
	}
	
	mCmdList->Close();

	mCmdListQueue.push(mCmdList);

	// Next frame
	mCurrFrameIndex = (mCurrFrameIndex + 1) % Settings::sQueuedFrameCount;
}

bool BasicCmdListRecorder::ValidateData() const noexcept {
	const std::size_t numGeomData{ mGeometryDataVec.size() };
	for (std::size_t i = 0UL; i < numGeomData; ++i) {
		const std::size_t numMatrices{ mGeometryDataVec[i].mWorldMatrices.size() };
		if (numMatrices == 0UL) {
			return false;
		}
	}

	for (std::uint32_t i = 0UL; i < Settings::sQueuedFrameCount; ++i) {
		if (mFrameCBuffer[i] == nullptr) {
			return false;
		}
	}

	const bool result = 
		CmdListRecorder::ValidateData() &&
		mObjectCBuffer != nullptr &&
		mObjectCBufferGpuDescHandleBegin.ptr != 0UL &&
		numGeomData != 0UL &&
		mMaterialsCBuffer != nullptr && 
		mMaterialsCBufferGpuDescHandleBegin.ptr != 0UL;

	return result;
}

void BasicCmdListRecorder::BuildBuffers(const Material* materials, const std::uint32_t numMaterials) noexcept {
	ASSERT(materials != nullptr);
	ASSERT(numMaterials != 0UL);

	ASSERT(mCbvSrvUavDescHeap == nullptr);
#ifdef _DEBUG
	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		ASSERT(mFrameCBuffer[i] == nullptr);
	}
#endif
	ASSERT(mObjectCBuffer == nullptr);
	ASSERT(mMaterialsCBuffer == nullptr);

	// Create CBV_SRV_UAV cbuffer descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0U;
	descHeapDesc.NumDescriptors = numMaterials * 2; // 1 obj cbuffer + 1 material cbuffer per geometry to draw
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ResourceManager::Get().CreateDescriptorHeap(descHeapDesc, mCbvSrvUavDescHeap);

	// Create object cbuffer and fill it
	const std::size_t objCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4)) };
	ResourceManager::Get().CreateUploadBuffer(objCBufferElemSize, numMaterials, mObjectCBuffer);
	mObjectCBufferGpuDescHandleBegin = mCbvSrvUavDescHeap->GetGPUDescriptorHandleForHeapStart();
	std::uint32_t k = 0U;
	const std::size_t numGeomData{ mGeometryDataVec.size() };
	for (std::size_t i = 0UL; i < numGeomData; ++i) {
		GeometryData& geomData{ mGeometryDataVec[i] };
		const std::uint32_t worldMatsCount{ (std::uint32_t)geomData.mWorldMatrices.size() };
		for (std::uint32_t j = 0UL; j < worldMatsCount; ++j) {
			DirectX::XMFLOAT4X4 w;
			const DirectX::XMMATRIX wMatrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&geomData.mWorldMatrices[j]));
			DirectX::XMStoreFloat4x4(&w, wMatrix);
			mObjectCBuffer->CopyData(k + j, &w, sizeof(w));
		}

		k += worldMatsCount;
	}

	// Create materials cbuffer		
	const std::size_t matCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(Material)) };
	ResourceManager::Get().CreateUploadBuffer(matCBufferElemSize, numMaterials, mMaterialsCBuffer);
	const std::size_t descHandleIncSize{ ResourceManager::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	mMaterialsCBufferGpuDescHandleBegin.ptr = mObjectCBufferGpuDescHandleBegin.ptr + numMaterials * descHandleIncSize;

	// Create object cbuffer descriptors
	// Create material cbuffer descriptors
	// Fill materials cbuffers data
	D3D12_GPU_VIRTUAL_ADDRESS materialsGpuAddress{ mMaterialsCBuffer->Resource()->GetGPUVirtualAddress() };
	D3D12_GPU_VIRTUAL_ADDRESS objCBufferGpuAddress{ mObjectCBuffer->Resource()->GetGPUVirtualAddress() };
	D3D12_CPU_DESCRIPTOR_HANDLE currObjCBufferDescHandle(mCbvSrvUavDescHeap->GetCPUDescriptorHandleForHeapStart());
	D3D12_CPU_DESCRIPTOR_HANDLE currMaterialCBufferDescHandle{ mCbvSrvUavDescHeap->GetCPUDescriptorHandleForHeapStart().ptr + numMaterials * descHandleIncSize };
	for (std::size_t i = 0UL; i < numMaterials; ++i) {
		// Create object cbuffers descriptors
		D3D12_CONSTANT_BUFFER_VIEW_DESC cBufferDesc{};
		cBufferDesc.BufferLocation = objCBufferGpuAddress + i * objCBufferElemSize;
		cBufferDesc.SizeInBytes = (std::uint32_t)objCBufferElemSize;
		ResourceManager::Get().CreateConstantBufferView(cBufferDesc, currObjCBufferDescHandle);

		// Create materials CBuffer descriptor
		cBufferDesc.BufferLocation = materialsGpuAddress + i * matCBufferElemSize;
		cBufferDesc.SizeInBytes = (std::uint32_t)matCBufferElemSize;
		ResourceManager::Get().CreateConstantBufferView(cBufferDesc, currMaterialCBufferDescHandle);

		mMaterialsCBuffer->CopyData((std::uint32_t)i, &materials[i], sizeof(Material));

		currMaterialCBufferDescHandle.ptr += descHandleIncSize;
		currObjCBufferDescHandle.ptr += descHandleIncSize;
	}

	// Create frame cbuffers
	const std::size_t frameCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4) * 2UL) };
	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		ResourceManager::Get().CreateUploadBuffer(frameCBufferElemSize, 1U, mFrameCBuffer[i]);
	}
}
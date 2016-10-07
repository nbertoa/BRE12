#include "HeightCmdListRecorder.h"

#include <DirectXMath.h>

#include <DXUtils\CBuffers.h>
#include <DXUtils/Material.h>
#include <MathUtils/MathUtils.h>
#include <PSOCreator/PSOCreator.h>
#include <ResourceManager/ResourceManager.h>
#include <ResourceManager/UploadBuffer.h>
#include <Utils/DebugUtils.h>

HeightCmdListRecorder::HeightCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue)
	: GeometryPassCmdListRecorder(device, cmdListQueue)
{
}

void HeightCmdListRecorder::Init(
	const GeometryData* geometryDataVec,
	const std::uint32_t numGeomData,
	const Material* materials,
	ID3D12Resource** textures,
	ID3D12Resource** normals,
	ID3D12Resource** heights,
	const std::uint32_t numResources,
	ID3D12Resource& cubeMap) noexcept
{
	ASSERT(ValidateData() == false);
	ASSERT(geometryDataVec != nullptr);
	ASSERT(numGeomData != 0U);
	ASSERT(materials != nullptr);
	ASSERT(numResources > 0UL);
	ASSERT(textures != nullptr);
	ASSERT(normals != nullptr);
	ASSERT(heights != nullptr);

	// Check that the total number of matrices (geometry to be drawn) will be equal to available materials
#ifdef _DEBUG
	std::size_t totalNumMatrices{ 0UL };
	for (std::size_t i = 0UL; i < numGeomData; ++i) {
		const std::size_t numMatrices{ geometryDataVec[i].mWorldMatrices.size() };
		totalNumMatrices += numMatrices;
		ASSERT(numMatrices != 0UL);
	}
	ASSERT(totalNumMatrices == numResources);
#endif
	mGeometryDataVec.reserve(numGeomData);
	for (std::uint32_t i = 0U; i < numGeomData; ++i) {
		mGeometryDataVec.push_back(geometryDataVec[i]);
	}

	const PSOCreator::PSOData& psoData(PSOCreator::CommonPSOData::GetData(PSOCreator::CommonPSOData::HEIGHT_MAPPING));

	mPSO = psoData.mPSO;
	mRootSign = psoData.mRootSign;

	BuildBuffers(materials, textures, normals, heights, numResources, cubeMap);

	ASSERT(ValidateData());
}

void HeightCmdListRecorder::RecordCommandLists(
	const FrameCBuffer& frameCBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE* geomPassRtvCpuDescHandles,
	const std::uint32_t geomPassRtvCpuDescHandlesCount,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept {
	ASSERT(ValidateData());
	ASSERT(geomPassRtvCpuDescHandles != nullptr);
	ASSERT(geomPassRtvCpuDescHandlesCount > 0);

	ID3D12CommandAllocator* cmdAlloc{ mCmdAlloc[mCurrFrameIndex] };
	ASSERT(cmdAlloc != nullptr);

	// Update frame constants
	UploadBuffer& uploadFrameCBuffer(*mFrameCBuffer[mCurrFrameIndex]);
	uploadFrameCBuffer.CopyData(0U, &frameCBuffer, sizeof(frameCBuffer));

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
	D3D12_GPU_DESCRIPTOR_HANDLE texturesBufferGpuDescHandle(mTexturesBufferGpuDescHandleBegin);
	D3D12_GPU_DESCRIPTOR_HANDLE normalsBufferGpuDescHandle(mNormalsBufferGpuDescHandleBegin);
	D3D12_GPU_DESCRIPTOR_HANDLE heightsBufferGpuDescHandle(mHeightsBufferGpuDescHandleBegin);

	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

	// Set frame constants root parameters
	D3D12_GPU_VIRTUAL_ADDRESS frameCBufferGpuVAddress(uploadFrameCBuffer.Resource()->GetGPUVirtualAddress());
	mCmdList->SetGraphicsRootConstantBufferView(1U, frameCBufferGpuVAddress);
	mCmdList->SetGraphicsRootConstantBufferView(2U, frameCBufferGpuVAddress);
	mCmdList->SetGraphicsRootConstantBufferView(6U, frameCBufferGpuVAddress);

	// Set immutable constants root parameters
	D3D12_GPU_VIRTUAL_ADDRESS immutableCBufferGpuVAddress(mImmutableCBuffer->Resource()->GetGPUVirtualAddress());
	mCmdList->SetGraphicsRootConstantBufferView(5U, immutableCBufferGpuVAddress);

	// Set cube map root parameter
	mCmdList->SetGraphicsRootDescriptorTable(9U, mCubeMapBufferGpuDescHandleBegin);

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

			mCmdList->SetGraphicsRootDescriptorTable(3U, heightsBufferGpuDescHandle);
			heightsBufferGpuDescHandle.ptr += descHandleIncSize;

			mCmdList->SetGraphicsRootDescriptorTable(4U, materialsCBufferGpuDescHandle);
			materialsCBufferGpuDescHandle.ptr += descHandleIncSize;

			mCmdList->SetGraphicsRootDescriptorTable(7U, texturesBufferGpuDescHandle);
			texturesBufferGpuDescHandle.ptr += descHandleIncSize;

			mCmdList->SetGraphicsRootDescriptorTable(8U, normalsBufferGpuDescHandle);
			normalsBufferGpuDescHandle.ptr += descHandleIncSize;
			
			mCmdList->DrawIndexedInstanced(geomData.mIndexBufferData.mCount, 1U, 0U, 0U, 0U);
		}
	}

	mCmdList->Close();

	mCmdListQueue.push(mCmdList);

	// Next frame
	mCurrFrameIndex = (mCurrFrameIndex + 1) % Settings::sQueuedFrameCount;
}

bool HeightCmdListRecorder::ValidateData() const noexcept {
	const bool result =
		GeometryPassCmdListRecorder::ValidateData() &&
		mTexturesBufferGpuDescHandleBegin.ptr != 0UL &&
		mNormalsBufferGpuDescHandleBegin.ptr != 0UL &&
		mHeightsBufferGpuDescHandleBegin.ptr != 0UL &&
		mCubeMapBufferGpuDescHandleBegin.ptr != 0UL;

	return result;
}

void HeightCmdListRecorder::BuildBuffers(
	const Material* materials,
	ID3D12Resource** textures,
	ID3D12Resource** normals,
	ID3D12Resource** heights,
	const std::uint32_t dataCount,
	ID3D12Resource& cubeMap) noexcept {
	ASSERT(materials != nullptr);
	ASSERT(textures != nullptr);
	ASSERT(normals != nullptr);
	ASSERT(heights != nullptr);
	ASSERT(dataCount != 0UL);

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
	// (1 obj cbuffer + 1 material cbuffer + 1 texture + 1 normal texture+ 1 height texture) per geometry + 1 cube map
	descHeapDesc.NumDescriptors = dataCount * 5U + 1U;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ResourceManager::Get().CreateDescriptorHeap(descHeapDesc, mCbvSrvUavDescHeap);

	// Create object cbuffer and fill it
	const std::size_t objCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(ObjectCBuffer)) };
	ResourceManager::Get().CreateUploadBuffer(objCBufferElemSize, dataCount, mObjectCBuffer);
	mObjectCBufferGpuDescHandleBegin = mCbvSrvUavDescHeap->GetGPUDescriptorHandleForHeapStart();
	std::uint32_t k = 0U;
	const std::size_t numGeomData{ mGeometryDataVec.size() };
	ObjectCBuffer objCBuffer;
	for (std::size_t i = 0UL; i < numGeomData; ++i) {
		GeometryData& geomData{ mGeometryDataVec[i] };
		const std::uint32_t worldMatsCount{ (std::uint32_t)geomData.mWorldMatrices.size() };
		for (std::uint32_t j = 0UL; j < worldMatsCount; ++j) {
			const DirectX::XMMATRIX wMatrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&geomData.mWorldMatrices[j]));
			DirectX::XMStoreFloat4x4(&objCBuffer.mWorld, wMatrix);
			mObjectCBuffer->CopyData(k + j, &objCBuffer, sizeof(objCBuffer));
		}

		k += worldMatsCount;
	}

	// Create materials cbuffer		
	const std::size_t matCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(Material)) };
	ResourceManager::Get().CreateUploadBuffer(matCBufferElemSize, dataCount, mMaterialsCBuffer);
	const std::size_t descHandleIncSize{ ResourceManager::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	mMaterialsCBufferGpuDescHandleBegin.ptr = mObjectCBufferGpuDescHandleBegin.ptr + dataCount * descHandleIncSize;

	// Set begin for textures in GPU
	mTexturesBufferGpuDescHandleBegin.ptr = mObjectCBufferGpuDescHandleBegin.ptr + dataCount * 2U * descHandleIncSize;

	// Set begin for normals in GPU
	mNormalsBufferGpuDescHandleBegin.ptr = mObjectCBufferGpuDescHandleBegin.ptr + dataCount * 3U * descHandleIncSize;

	// Set begin for heights in GPU
	mHeightsBufferGpuDescHandleBegin.ptr = mObjectCBufferGpuDescHandleBegin.ptr + dataCount * 4U * descHandleIncSize;

	// Create object cbuffer descriptors
	// Create material cbuffer descriptors
	// Fill materials cbuffers data
	D3D12_GPU_VIRTUAL_ADDRESS materialsGpuAddress{ mMaterialsCBuffer->Resource()->GetGPUVirtualAddress() };
	D3D12_GPU_VIRTUAL_ADDRESS objCBufferGpuAddress{ mObjectCBuffer->Resource()->GetGPUVirtualAddress() };
	D3D12_CPU_DESCRIPTOR_HANDLE currObjCBufferDescHandle(mCbvSrvUavDescHeap->GetCPUDescriptorHandleForHeapStart());
	D3D12_CPU_DESCRIPTOR_HANDLE currMaterialCBufferDescHandle{ mCbvSrvUavDescHeap->GetCPUDescriptorHandleForHeapStart().ptr + dataCount * descHandleIncSize };
	D3D12_CPU_DESCRIPTOR_HANDLE currTextureBufferDescHandle{ mCbvSrvUavDescHeap->GetCPUDescriptorHandleForHeapStart().ptr + dataCount * 2U * descHandleIncSize };
	D3D12_CPU_DESCRIPTOR_HANDLE currNormalsBufferDescHandle{ mCbvSrvUavDescHeap->GetCPUDescriptorHandleForHeapStart().ptr + dataCount * 3U * descHandleIncSize };
	D3D12_CPU_DESCRIPTOR_HANDLE currHeightsBufferDescHandle{ mCbvSrvUavDescHeap->GetCPUDescriptorHandleForHeapStart().ptr + dataCount * 4U * descHandleIncSize };

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	for (std::size_t i = 0UL; i < dataCount; ++i) {
		// Create object cbuffers descriptors
		D3D12_CONSTANT_BUFFER_VIEW_DESC cBufferDesc{};
		cBufferDesc.BufferLocation = objCBufferGpuAddress + i * objCBufferElemSize;
		cBufferDesc.SizeInBytes = (std::uint32_t)objCBufferElemSize;
		ResourceManager::Get().CreateConstantBufferView(cBufferDesc, currObjCBufferDescHandle);

		// Create materials CBuffer descriptor
		cBufferDesc.BufferLocation = materialsGpuAddress + i * matCBufferElemSize;
		cBufferDesc.SizeInBytes = (std::uint32_t)matCBufferElemSize;
		ResourceManager::Get().CreateConstantBufferView(cBufferDesc, currMaterialCBufferDescHandle);

		// Create texture descriptor
		ID3D12Resource* res{ textures[i] };
		srvDesc.Format = res->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = res->GetDesc().MipLevels;
		ResourceManager::Get().CreateShaderResourceView(*res, srvDesc, currTextureBufferDescHandle);

		// Create normal texture descriptor
		res = normals[i];
		srvDesc.Format = res->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = res->GetDesc().MipLevels;
		ResourceManager::Get().CreateShaderResourceView(*res, srvDesc, currNormalsBufferDescHandle);

		// Create height texture descriptor
		res = heights[i];
		srvDesc.Format = res->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = res->GetDesc().MipLevels;
		ResourceManager::Get().CreateShaderResourceView(*res, srvDesc, currHeightsBufferDescHandle);

		mMaterialsCBuffer->CopyData((std::uint32_t)i, &materials[i], sizeof(Material));

		currMaterialCBufferDescHandle.ptr += descHandleIncSize;
		currObjCBufferDescHandle.ptr += descHandleIncSize;
		currTextureBufferDescHandle.ptr += descHandleIncSize;
		currNormalsBufferDescHandle.ptr += descHandleIncSize;
		currHeightsBufferDescHandle.ptr += descHandleIncSize;
	}

	// Create frame cbuffers
	const std::size_t frameCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(FrameCBuffer)) };
	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		ResourceManager::Get().CreateUploadBuffer(frameCBufferElemSize, 1U, mFrameCBuffer[i]);
	}

	// Create immutable cbuffer
	const std::size_t immutableCBufferElemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(ImmutableCBuffer)) };
	ResourceManager::Get().CreateUploadBuffer(immutableCBufferElemSize, 1U, mImmutableCBuffer);
	ImmutableCBuffer immutableCBuffer;
	mImmutableCBuffer->CopyData(0U, &immutableCBuffer, sizeof(immutableCBuffer));

	// Set begin for cube map in GPU	
	mCubeMapBufferGpuDescHandleBegin.ptr = mObjectCBufferGpuDescHandleBegin.ptr + dataCount * 5U * descHandleIncSize;

	// Create cube map texture descriptor
	srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = cubeMap.GetDesc().MipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = cubeMap.GetDesc().Format;
	const D3D12_CPU_DESCRIPTOR_HANDLE cubeMapBufferDescHandle{ mCbvSrvUavDescHeap->GetCPUDescriptorHandleForHeapStart().ptr + dataCount * 5U * descHandleIncSize };
	ResourceManager::Get().CreateShaderResourceView(cubeMap, srvDesc, cubeMapBufferDescHandle);
}
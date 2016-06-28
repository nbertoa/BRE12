#include "ShapeInitTask.h"

#include <ResourceManager/ResourceManager.h>
#include <ResourceManager/UploadBuffer.h>
#include <Utils/DebugUtils.h>

void ShapeInitTask::Execute(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdLists, CmdBuilderTaskInput& output) noexcept {
	InitTask::Execute(device, cmdLists, output);

	ASSERT(output.mGeomDataVec.empty());
	ASSERT(output.mCmdList != nullptr);
	ASSERT(output.mCmdAlloc[0] != nullptr);
	ASSERT(output.mPSO != nullptr);

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	CHECK_HR(output.mCmdAlloc[0]->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	CHECK_HR(output.mCmdList->Reset(output.mCmdAlloc[0], output.mPSO));

	const std::size_t numMeshes{ mInput.mMeshInfoVec.size() };
	output.mGeomDataVec.reserve(numMeshes);

	const float baseOffset{ 10.0f };
	for (std::size_t i = 0UL; i < numMeshes; ++i) {
		const MeshInfo& meshInfo{ mInput.mMeshInfoVec[i] };
		ASSERT(meshInfo.ValidateData());
		GeometryData geomData;
		BuildVertexAndIndexBuffers(
			geomData, 
			meshInfo.mVerts, 
			meshInfo.mNumVerts, 
			sizeof(GeometryGenerator::Vertex), 
			meshInfo.mIndices, 
			meshInfo.mNumIndices,
			*output.mCmdList);
		geomData.mWorld = meshInfo.mWorld;
		output.mGeomDataVec.push_back(geomData);
	}

	output.mCmdList->Close();
	cmdLists.push(output.mCmdList);

	BuildConstantBuffers(device, output);
}

void ShapeInitTask::BuildConstantBuffers(ID3D12Device& device, CmdBuilderTaskInput& output) noexcept {
	ASSERT(output.mCBVHeap == nullptr);
	ASSERT(!output.mGeomDataVec.empty());
	ASSERT(output.mFrameConstants == nullptr);
	ASSERT(output.mObjectConstants == nullptr);

	const std::uint32_t geomCount{ (std::uint32_t)output.mGeomDataVec.size() };

	// Create constant buffers descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0U;
	descHeapDesc.NumDescriptors = geomCount + 1U; // +1 for frame constants
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ResourceManager::gManager->CreateDescriptorHeap(descHeapDesc, output.mCBVHeap);

	const std::size_t descHandleIncSize{ device.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };

	const std::size_t elemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4)) };

	// Fill constant buffers descriptor heap with per objects constants buffer views
	ResourceManager::gManager->CreateUploadBuffer(elemSize, geomCount, output.mObjectConstants);	
	D3D12_GPU_VIRTUAL_ADDRESS cbObjGPUBaseAddress{ output.mObjectConstants->Resource()->GetGPUVirtualAddress() };	
	for (std::size_t i = 0UL; i < geomCount; ++i) {
		D3D12_CPU_DESCRIPTOR_HANDLE descHandle = output.mCBVHeap->GetCPUDescriptorHandleForHeapStart();
		descHandle.ptr += i * descHandleIncSize;

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
		cbvDesc.BufferLocation = cbObjGPUBaseAddress + i * elemSize;
		cbvDesc.SizeInBytes = (std::uint32_t)elemSize;

		ResourceManager::gManager->CreateConstantBufferView(cbvDesc, descHandle);
	}

	// Fill constnat buffers descriptor heap with per frame constant buffer view
	ResourceManager::gManager->CreateUploadBuffer(elemSize, 1U, output.mFrameConstants);
	D3D12_GPU_VIRTUAL_ADDRESS cbFrameGPUBaseAddress{ output.mFrameConstants->Resource()->GetGPUVirtualAddress() };
	D3D12_CPU_DESCRIPTOR_HANDLE descHandle = output.mCBVHeap->GetCPUDescriptorHandleForHeapStart();
	descHandle.ptr += geomCount * descHandleIncSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = cbFrameGPUBaseAddress;
	cbvDesc.SizeInBytes = (std::uint32_t)elemSize;
	ResourceManager::gManager->CreateConstantBufferView(cbvDesc, descHandle);
}
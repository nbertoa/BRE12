#include "ShapeInitTask.h"

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

void ShapeInitTask::Execute(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdLists, CmdBuilderTaskInput& output) noexcept {
	InitTask::Execute(device, cmdLists, output);

	ASSERT(output.mGeomDataVec.empty());
	ASSERT(output.mCmdList1 != nullptr);
	ASSERT(output.mCmdAlloc1 != nullptr);	
	ASSERT(output.mCmdList2 != nullptr);
	ASSERT(output.mCmdAlloc2 != nullptr);
	ASSERT(output.mPSO != nullptr);

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	CHECK_HR(output.mCmdAlloc1->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	CHECK_HR(output.mCmdList1->Reset(output.mCmdAlloc1, output.mPSO));

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
			sizeof(Vertex), 
			meshInfo.mIndices, 
			meshInfo.mNumIndices,
			*output.mCmdList1);
		geomData.mWorld = meshInfo.mWorld;
		output.mGeomDataVec.push_back(geomData);
	}

	output.mCmdList1->Close();
	cmdLists.push(output.mCmdList1);

	BuildConstantBuffers(device, output);
}

void ShapeInitTask::BuildConstantBuffers(ID3D12Device& device, CmdBuilderTaskInput& output) noexcept {
	ASSERT(output.mCBVHeap == nullptr);
	ASSERT(!output.mGeomDataVec.empty());
	ASSERT(output.mObjectConstants == nullptr);

	const std::uint32_t geomCount{ (std::uint32_t)output.mGeomDataVec.size() };

	// Create constant buffers descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0U;
	descHeapDesc.NumDescriptors = geomCount;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	CHECK_HR(device.CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&output.mCBVHeap)));

	const std::size_t elemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4)) };
	ResourceManager::gManager->CreateUploadBuffer(elemSize, geomCount, output.mObjectConstants);
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress{ output.mObjectConstants->Resource()->GetGPUVirtualAddress() };
	const std::size_t descHandleIncSize{ device.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	for (std::size_t i = 0UL; i < geomCount; ++i) {
		D3D12_CPU_DESCRIPTOR_HANDLE descHandle = output.mCBVHeap->GetCPUDescriptorHandleForHeapStart();
		descHandle.ptr += i * descHandleIncSize;

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
		cbvDesc.BufferLocation = cbAddress + i * elemSize;
		cbvDesc.SizeInBytes = (std::uint32_t)elemSize;

		device.CreateConstantBufferView(&cbvDesc, descHandle);
	}
}
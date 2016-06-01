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

ShapeTask::ShapeTask(const char* taskName, ID3D12Device* device, const D3D12_VIEWPORT& screenViewport, const D3D12_RECT& scissorRect)
	: RenderTask(taskName, device, screenViewport, scissorRect)
{
	ASSERT(taskName != nullptr);
	ASSERT(device != nullptr);
}

void ShapeTask::Init(const RenderTaskInitData& initData, tbb::concurrent_vector<ID3D12CommandList*>& cmdLists) noexcept {
	RenderTask::Init(initData, cmdLists);
	
	ASSERT(mGeomDataVec.empty());
	ASSERT(mCmdListAllocator.Get() != nullptr);
	ASSERT(mCmdList.Get() != nullptr);
	ASSERT(mPSO != nullptr);

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	CHECK_HR(mCmdListAllocator->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	CHECK_HR(mCmdList->Reset(mCmdListAllocator.Get(), mPSO));

	const std::size_t numMeshes{ initData.mMeshInfoVec.size() };
	mGeomDataVec.reserve(numMeshes);
	
	const float baseOffset{ 10.0f };
	for (std::size_t i = 0UL; i < numMeshes; ++i) {
		const MeshInfo& meshInfo{ initData.mMeshInfoVec[i] };
		ASSERT(meshInfo.mData != nullptr);
		const GeometryGenerator::MeshData& meshData{ *meshInfo.mData };
		const std::uint32_t numVertices = (std::uint32_t)meshData.mVertices.size();
		std::vector<Vertex> vertices;
		vertices.reserve(numVertices);
		for (const GeometryGenerator::Vertex& vtx : meshData.mVertices) {
			vertices.push_back(Vertex{ DirectX::XMFLOAT4(vtx.mPosition.x, vtx.mPosition.y, vtx.mPosition.z, 1.0f) });
		}

		GeometryData geomData;
		BuildVertexAndIndexBuffers(geomData, vertices.data(), numVertices, sizeof(Vertex), meshData.mIndices32.data(), (std::uint32_t)meshData.mIndices32.size());
		geomData.mWorld = meshInfo.mWorld;
		mGeomDataVec.push_back(geomData);
	}

	mCmdList->Close();
	cmdLists.push_back(mCmdList.Get());

	BuildConstantBuffers();
}

void ShapeTask::Update() noexcept {
	ASSERT(mObjectConstants != nullptr);

	const DirectX::XMMATRIX vpMatrix = DirectX::XMMatrixTranspose(Camera::gCamera->GetViewProj());

	const std::uint32_t geomCount{ (std::uint32_t)mGeomDataVec.size() };
	for (std::uint32_t i = 0U; i < geomCount; ++i) {
		const GeometryData& geomData{ mGeomDataVec[i] };
		DirectX::XMFLOAT4X4 wvpMatrix{};
		const DirectX::XMMATRIX wMatrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&geomData.mWorld));
		DirectX::XMStoreFloat4x4(&wvpMatrix, DirectX::XMMatrixMultiply(vpMatrix, wMatrix));
		mObjectConstants->CopyData(i, &wvpMatrix, sizeof(wvpMatrix));
	}	
}

void ShapeTask::BuildCmdLists(
	tbb::concurrent_vector<ID3D12CommandList*>& cmdLists,
	const D3D12_CPU_DESCRIPTOR_HANDLE& backBufferHandle,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept {
	ASSERT(mCmdList.Get() != nullptr);
	ASSERT(mCmdListAllocator.Get() != nullptr);
	ASSERT(mCBVHeap != nullptr);
	ASSERT(mRootSign != nullptr);
	ASSERT(mPSO != nullptr);

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	CHECK_HR(mCmdListAllocator->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	CHECK_HR(mCmdList->Reset(mCmdListAllocator.Get(), mPSO));

	mCmdList->RSSetViewports(1U, &mViewport);
	mCmdList->RSSetScissorRects(1U, &mScissorRect);
	mCmdList->OMSetRenderTargets(1U, &backBufferHandle, true, &depthStencilHandle);

	mCmdList->SetDescriptorHeaps(1U, &mCBVHeap);
	mCmdList->SetGraphicsRootSignature(mRootSign);
	D3D12_GPU_DESCRIPTOR_HANDLE cbvGpuDescHandle = mCBVHeap->GetGPUDescriptorHandleForHeapStart();
	const std::size_t descHandleIncSize{ mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };	
	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const std::size_t geomCount{ mGeomDataVec.size() };
	for (std::size_t i = 0UL; i < geomCount; ++i) {
		mCmdList->SetGraphicsRootDescriptorTable(0U, cbvGpuDescHandle);

		const GeometryData& geomData{ mGeomDataVec[i] };
		mCmdList->IASetVertexBuffers(0U, 1U, &geomData.mVertexBufferView);
		mCmdList->IASetIndexBuffer(&geomData.mIndexBufferView);		
		mCmdList->DrawIndexedInstanced(geomData.mIndexCount, 1U, geomData.mStartIndexLoc, geomData.mBaseVertexLoc, 0U);
		cbvGpuDescHandle.ptr += descHandleIncSize;
	}

	mCmdList->Close();

	cmdLists.push_back(mCmdList.Get());
}

void ShapeTask::BuildConstantBuffers() noexcept {
	ASSERT(mCBVHeap == nullptr);
	ASSERT(mDevice != nullptr);
	ASSERT(!mGeomDataVec.empty());
	ASSERT(mObjectConstants == nullptr);

	const std::uint32_t geomCount{ (std::uint32_t)mGeomDataVec.size() };

	// Create constant buffers descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0U;
	descHeapDesc.NumDescriptors = geomCount;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	
	CHECK_HR(mDevice->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&mCBVHeap)));

	const std::size_t elemSize{ UploadBuffer::CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4)) };	
	const std::string name{ mTaskName + "_cbv" };
	ResourceManager::gManager->CreateUploadBuffer(name.c_str(), elemSize, geomCount, mObjectConstants);
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress{ mObjectConstants->Resource()->GetGPUVirtualAddress() };
	const std::size_t descHandleIncSize{ mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	for (std::size_t i = 0UL; i < geomCount; ++i) {
		D3D12_CPU_DESCRIPTOR_HANDLE descHandle = mCBVHeap->GetCPUDescriptorHandleForHeapStart();
		descHandle.ptr += i * descHandleIncSize;

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
		cbvDesc.BufferLocation = cbAddress + i * elemSize;
		cbvDesc.SizeInBytes = (std::uint32_t)elemSize;

		mDevice->CreateConstantBufferView(&cbvDesc, descHandle);
	}
}
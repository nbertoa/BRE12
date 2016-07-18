#include "GeomBuffersCreator.h"

#include <tbb/concurrent_queue.h>

#include <ResourceManager/ResourceManager.h>
#include <Utils/DebugUtils.h>

namespace {
	void BuildVertexAndIndexBuffers(ID3D12GraphicsCommandList& cmdList, const GeomBuffersCreator::Input& input, GeomBuffersCreator::Output& output) {
		ASSERT(input.ValidateData());

		std::uint32_t byteSize{ input.mNumVerts * (std::uint32_t)input.mVertexSize };

		ResourceManager::gManager->CreateDefaultBuffer(cmdList, input.mVertsData, byteSize, output.mVertexBuffer, output.mUploadVertexBuffer);
		output.mVertexBufferView.BufferLocation = output.mVertexBuffer->GetGPUVirtualAddress();
		output.mVertexBufferView.SizeInBytes = byteSize;
		output.mVertexBufferView.StrideInBytes = (std::uint32_t)input.mVertexSize;

		output.mIndexCount = input.mNumIndices;
		byteSize = input.mNumIndices * sizeof(std::uint32_t);
		ResourceManager::gManager->CreateDefaultBuffer(cmdList, input.mIndexData, byteSize, output.mIndexBuffer, output.mUploadIndexBuffer);
		output.mIndexBufferView.BufferLocation = output.mIndexBuffer->GetGPUVirtualAddress();
		output.mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
		output.mIndexBufferView.SizeInBytes = byteSize;
	}
}

namespace GeomBuffersCreator {
	Input::Input(const void* verts, const std::uint32_t numVerts, const std::size_t vertexSize, const void* indices, const std::uint32_t numIndices)
		: mVertsData(verts)
		, mNumVerts(numVerts)
		, mVertexSize(vertexSize)
		, mIndexData(indices)
		, mNumIndices(numIndices)
	{
		ASSERT(mVertsData != nullptr);
		ASSERT(mNumVerts != 0U);
		ASSERT(mVertexSize != 0UL);
		ASSERT(mIndexData != nullptr);
		ASSERT(mNumIndices != 0U);
	}

	bool Input::ValidateData() const noexcept {
		return mVertsData != nullptr && mNumVerts != 0U && mVertexSize != 0UL && mIndexData != nullptr && mNumIndices != 0U;
	}

	void Execute(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue, ID3D12GraphicsCommandList& cmdList, const Input& input, Output& output) noexcept {
		ASSERT(input.ValidateData());
		BuildVertexAndIndexBuffers(cmdList, input, output);
		cmdList.Close();
		cmdListQueue.push(&cmdList);
	}
}




#pragma once

#include <cstdint>

#include <GeometryGenerator/GeometryGenerator.h>
#include <ResourceManager\BufferCreator.h>
#include <Utils/DebugUtils.h>

struct aiMesh;
struct ID3D12GraphicsCommandList;
class Model;

// Stores model's mesh vertex and buffer data.
// It used by model class.
class Mesh {
	friend class Model;

public:
	__forceinline const BufferCreator::VertexBufferData& VertexBufferData() const noexcept { ASSERT(mVertexBufferData.ValidateData()); return mVertexBufferData; }
	__forceinline const BufferCreator::IndexBufferData& IndexBufferData() const noexcept { ASSERT(mIndexBufferData.ValidateData()); return mIndexBufferData; }

private:
	// Command lists are used to store buffers creation (vertex and index per mesh)
	// cmdList must be in recorded state before calling these method.
	// cmdList must be executed after calling these methods, to create the commited resource.
	explicit Mesh(
		const aiMesh& mesh, 
		ID3D12GraphicsCommandList& cmdList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer);

	explicit Mesh(
		const GeometryGenerator::MeshData& meshData, 
		ID3D12GraphicsCommandList& cmdList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer);

	BufferCreator::VertexBufferData mVertexBufferData;
	BufferCreator::IndexBufferData mIndexBufferData;
};
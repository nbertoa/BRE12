#pragma once

#include <cstdint>

#include <GeometryGenerator/GeometryGenerator.h>
#include <ResourceManager\BufferCreator.h>
#include <Utils/DebugUtils.h>

struct aiMesh;
struct ID3D12GraphicsCommandList;
class Model;

// Stores model's mesh vertex and buffer data.
class Mesh {
	friend class Model;

public:
	~Mesh() = default;
	Mesh(const Mesh&) = delete;
	const Mesh& operator=(const Mesh&) = delete;
	Mesh(Mesh&&) = default;
	Mesh& operator=(Mesh&&) = delete;

	// Preconditions:
	// - data must be valid
	__forceinline const BufferCreator::VertexBufferData& GetVertexBufferData() const noexcept {
		ASSERT(mVertexBufferData.IsDataValid());
		return mVertexBufferData;
	}

	// Preconditions:
	// - data must be valid
	__forceinline const BufferCreator::IndexBufferData& GetIndexBufferData() const noexcept {
		ASSERT(mIndexBufferData.IsDataValid());
		return mIndexBufferData;
	}

private:
	// Command lists are used to store buffers creation (vertex and index per mesh)
	// "commandList" must be executed after calling these methods, to create the commited resource.
	// Preconditions:
	// - "commandList" must be in recorded state before calling these method.
	explicit Mesh(
		const aiMesh& mesh, 
		ID3D12GraphicsCommandList& commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer);

	explicit Mesh(
		const GeometryGenerator::MeshData& meshData, 
		ID3D12GraphicsCommandList& commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer);
	
	BufferCreator::VertexBufferData mVertexBufferData;
	BufferCreator::IndexBufferData mIndexBufferData;
};
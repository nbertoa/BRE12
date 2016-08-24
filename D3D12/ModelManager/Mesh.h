#pragma once

#include <cstdint>

#include <GeometryGenerator/GeometryGenerator.h>
#include <ResourceManager\BufferCreator.h>
#include <Utils/DebugUtils.h>

struct aiMesh;
struct ID3D12GraphicsCommandList;
class Model;

class Mesh {
	friend class Model;

public:
	Mesh(const Mesh& rhs) = delete;
	Mesh& operator=(const Mesh& rhs) = delete;

	__forceinline BufferCreator::VertexBufferData& VertexBufferData() noexcept { ASSERT(mVertexBufferData.ValidateData()); return mVertexBufferData; }
	__forceinline BufferCreator::IndexBufferData& IndexBufferData() noexcept { ASSERT(mIndexBufferData.ValidateData()); return mIndexBufferData; }

private:
	explicit Mesh(const aiMesh& mesh, ID3D12GraphicsCommandList& cmdList);
	explicit Mesh(const GeometryGenerator::MeshData& meshData, ID3D12GraphicsCommandList& cmdList);

	BufferCreator::VertexBufferData mVertexBufferData;
	BufferCreator::IndexBufferData mIndexBufferData;
};
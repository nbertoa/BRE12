#pragma once

#include <vector>

#include <GeometryGenerator/GeometryGenerator.h>

struct ID3D12GraphicsCommandList;
class Mesh;

class Model {
public:
	// Command lists are used to store buffers creation (vertex and index per mesh)
	// cmdList must be in recorded state before calling these method.
	// cmdList must be executed after calling these methods, to create the commited resource.
	explicit Model(const char* filename, ID3D12GraphicsCommandList& cmdList);
	explicit Model(const GeometryGenerator::MeshData& meshData, ID3D12GraphicsCommandList& cmdList);

	Model(const Model& rhs) = delete;
	Model& operator=(const Model& rhs) = delete;
	~Model();

	__forceinline bool HasMeshes() const noexcept { return (mMeshes.size() > 0UL); }
	__forceinline const std::vector<Mesh*>& Meshes() const noexcept { return mMeshes; }

private:
	std::vector<Mesh*> mMeshes;
};

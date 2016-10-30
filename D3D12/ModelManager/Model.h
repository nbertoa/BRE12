#pragma once

#include <vector>
#include <wrl.h>

#include <GeometryGenerator/GeometryGenerator.h>
#include <ModelManager/Mesh.h>

struct ID3D12GraphicsCommandList;
struct ID3D12Resource;

// - Load model data from a filepath.
// - Get meshes 
// It stores vertex/index data in Mesh class.
class Model {
public:
	// Command lists are used to store buffers creation (vertex and index per mesh)
	// cmdList must be in recorded state before calling these method.
	// cmdList must be executed after calling these methods, to create the commited resource.
	explicit Model(
		const char* filename, 
		ID3D12GraphicsCommandList& cmdList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer);

	explicit Model(
		const GeometryGenerator::MeshData& meshData, 
		ID3D12GraphicsCommandList& cmdList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer);

	Model(const Model& rhs) = delete;
	Model& operator=(const Model& rhs) = delete;

	__forceinline bool HasMeshes() const noexcept { return (mMeshes.size() > 0UL); }
	__forceinline const std::vector<Mesh>& Meshes() const noexcept { return mMeshes; }

private:
	std::vector<Mesh> mMeshes;
};

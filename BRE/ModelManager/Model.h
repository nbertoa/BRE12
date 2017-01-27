#pragma once

#include <vector>
#include <wrl.h>

#include <GeometryGenerator/GeometryGenerator.h>
#include <ModelManager/Mesh.h>

struct ID3D12GraphicsCommandList;
struct ID3D12Resource;

// - To load model data from a filepath.
// - To get meshes 
class Model {
public:
	~Model() = default;
	Model(const Model&) = delete;
	const Model& operator=(const Model&) = delete;
	Model(Model&&) = delete;
	Model& operator=(Model&&) = delete;

	// Command lists are used to store buffers creation (vertex and index per mesh)
	// Preconditions:
	// - "commandList" must be in recorded state before calling these method.
	// - "commandList" must be executed after calling these methods, to create the commited resource.
	explicit Model(
		const char* modelFilename,
		ID3D12GraphicsCommandList& commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer);

	explicit Model(
		const GeometryGenerator::MeshData& meshData,
		ID3D12GraphicsCommandList& commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer);

	__forceinline bool HasMeshes() const noexcept { return (mMeshes.size() > 0UL); }
	__forceinline const std::vector<Mesh>& GetMeshes() const noexcept { return mMeshes; }

private:
	std::vector<Mesh> mMeshes;
};

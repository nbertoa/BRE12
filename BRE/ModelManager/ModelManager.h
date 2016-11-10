#pragma once

#include <d3d12.h>
#include <memory>
#include <tbb\concurrent_hash_map.h>
#include <tbb/mutex.h>
#include <wrl.h>

#include <ModelManager/Model.h>

// This class is responsible to create/get/erase models or geometry
// - Models
// - Geometry
class ModelManager {
public:
	static ModelManager& Create() noexcept;
	static ModelManager& Get() noexcept;

	~ModelManager() = default;
	ModelManager(const ModelManager&) = delete;
	const ModelManager& operator=(const ModelManager&) = delete;
	ModelManager(ModelManager&&) = delete;
	ModelManager& operator=(ModelManager&&) = delete;

	// Returns id to get model after creation
	std::size_t LoadModel(
		const char* filename, 
		Model* &model, ID3D12GraphicsCommandList& cmdList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

	// Creates a box centered at the origin with the given dimensions, where each
	// face has m rows and n columns of vertices.
	std::size_t CreateBox(
		const float width, 
		const float height, 
		const float depth, 
		const std::uint32_t numSubdivisions, 
		Model* &model, 
		ID3D12GraphicsCommandList& cmdList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

	// Creates a sphere centered at the origin with the given radius.  The
	// slices and stacks parameters control the degree of tessellation.
	std::size_t CreateSphere(
		const float radius, 
		const std::uint32_t sliceCount, 
		const std::uint32_t stackCount, 
		Model* &model, 
		ID3D12GraphicsCommandList& cmdList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

	// Creates a geosphere centered at the origin with the given radius.  The
	// depth controls the level of tessellation.
	std::size_t CreateGeosphere(
		const float radius, 
		const std::uint32_t numSubdivisions, 
		Model* &model, 
		ID3D12GraphicsCommandList& cmdList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

	// Creates a cylinder parallel to the y-axis, and centered about the origin.  
	// The bottom and top radius can vary to form various cone shapes rather than true
	// cylinders.  The slices and stacks parameters control the degree of tessellation.
	std::size_t CreateCylinder(
		const float bottomRadius,
		const float topRadius,
		const float height, 
		const std::uint32_t sliceCount,
		const std::uint32_t stackCount,
		Model* &model,
		ID3D12GraphicsCommandList& cmdList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

	// Creates an mxn grid in the xz-plane with m rows and n columns, centered
	// at the origin with the specified width and depth.
	std::size_t CreateGrid(
		const float width, 
		const float depth, 
		const std::uint32_t m, 
		const std::uint32_t n, 
		Model* &model, 
		ID3D12GraphicsCommandList& cmdList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

	// Creates a quad aligned with the screen.  This is useful for post-processing and screen effects.
	std::size_t CreateQuad(
		const float x, 
		const float y, 
		const float w, 
		const float h,
		const float depth, 
		Model* &model, 
		ID3D12GraphicsCommandList& cmdList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

	// Creates a full screen quad aligned with the screen. This is useful for post-processing and screen effects.
	// Position coordinates will be in NDC.
	std::size_t CreateFullscreenQuad(
		Model* &model,
		ID3D12GraphicsCommandList& cmdList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

	// Asserts if id does not exist
	Model& GetModel(const std::size_t id) noexcept;

	// Asserts if id is not present
	void Erase(const std::size_t id) noexcept;

	// Invalidate all ids.
	__forceinline void Clear() noexcept { mModelById.clear(); }

private:
	ModelManager() = default;

	using ModelById = tbb::concurrent_hash_map<std::size_t, std::unique_ptr<Model>>;
	ModelById mModelById;

	tbb::mutex mMutex;
};

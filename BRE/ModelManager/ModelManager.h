#pragma once

#include <d3d12.h>
#include <memory>
#include <mutex>
#include <tbb\concurrent_hash_map.h>
#include <wrl.h>

#include <ModelManager/Model.h>

// To create/get models or built-in geometry.
class ModelManager {
public:
	ModelManager() = delete;
	~ModelManager() = delete;
	ModelManager(const ModelManager&) = delete;
	const ModelManager& operator=(const ModelManager&) = delete;
	ModelManager(ModelManager&&) = delete;
	ModelManager& operator=(ModelManager&&) = delete;

	// Returns id to get model after creation
	static std::size_t LoadModel(
		const char* modelFilename, 
		Model* &model, ID3D12GraphicsCommandList& commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

	// Geometry is centered at the origin.
	// Returns id of the model
	static std::size_t CreateBox(
		const float width, 
		const float height, 
		const float depth, 
		const std::uint32_t numSubdivisions, 
		Model* &model, 
		ID3D12GraphicsCommandList& commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

	// Geometry is centered at the origin.
	// Returns id of the model
	static std::size_t CreateSphere(
		const float radius, 
		const std::uint32_t sliceCount, 
		const std::uint32_t stackCount, 
		Model* &model, 
		ID3D12GraphicsCommandList& commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

	// Geometry is centered at the origin.
	// Returns id of the model
	static std::size_t CreateGeosphere(
		const float radius, 
		const std::uint32_t numSubdivisions, 
		Model* &model, 
		ID3D12GraphicsCommandList& commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

	// Creates a cylinder parallel to the y-axis, and centered about the origin.  
	// Returns id of the model
	static std::size_t CreateCylinder(
		const float bottomRadius,
		const float topRadius,
		const float height, 
		const std::uint32_t sliceCount,
		const std::uint32_t stackCount,
		Model* &model,
		ID3D12GraphicsCommandList& commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

	// Creates a rows x columns grid in the xz-plane centered
	// at the origin.
	// Returns id of the model.
	static std::size_t CreateGrid(
		const float width, 
		const float depth, 
		const std::uint32_t rows, 
		const std::uint32_t columns, 
		Model* &model, 
		ID3D12GraphicsCommandList& commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

	// Preconditions:
	// - id must be valid
	static Model& GetModel(const std::size_t id) noexcept;

private:
	using ModelById = tbb::concurrent_hash_map<std::size_t, std::unique_ptr<Model>>;
	static ModelById mModelById;

	static std::mutex mMutex;
};

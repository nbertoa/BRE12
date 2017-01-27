#include "ModelManager.h"

#include <GeometryGenerator\GeometryGenerator.h>
#include <Utils/DebugUtils.h>
#include <Utils\NumberGeneration.h>

namespace {

	std::unique_ptr<ModelManager> gManager{ nullptr };
}

ModelManager& ModelManager::Create() noexcept {
	ASSERT(gManager == nullptr);
	gManager.reset(new ModelManager());
	return *gManager.get();
}

ModelManager& ModelManager::Get() noexcept {
	ASSERT(gManager != nullptr);
	return *gManager.get();
}

std::size_t ModelManager::LoadModel(
	const char* modelFilename, 
	Model* &model, 
	ID3D12GraphicsCommandList& commandList,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept 
{
	ASSERT(modelFilename != nullptr);

	mMutex.lock();
	model = new Model(modelFilename, commandList, uploadVertexBuffer, uploadIndexBuffer);
	mMutex.unlock();

	const std::size_t id{ NumberGeneration::GetIncrementalSizeT() };
	ModelById::accessor accessor;
#ifdef _DEBUG
	mModelById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mModelById.insert(accessor, id);
	accessor->second.reset(model);
	accessor.release();

	return id;
}

std::size_t ModelManager::CreateBox(
	const float width, 
	const float height, 
	const float depth, 
	const std::uint32_t numSubdivisions,
	Model* &model, 
	ID3D12GraphicsCommandList& commandList,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept 
{
	GeometryGenerator::MeshData meshData;
	GeometryGenerator::CreateBox(width, height, depth, numSubdivisions, meshData);

	mMutex.lock();
	model = new Model(meshData, commandList, uploadVertexBuffer, uploadIndexBuffer);
	mMutex.unlock();

	const std::size_t id{ NumberGeneration::GetIncrementalSizeT() };
	ModelById::accessor accessor;
#ifdef _DEBUG
	mModelById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mModelById.insert(accessor, id);
	accessor->second.reset(model);
	accessor.release();

	return id;
}

std::size_t ModelManager::CreateSphere(
	const float radius, 
	const std::uint32_t sliceCount, 
	const std::uint32_t stackCount, 
	Model* &model, 
	ID3D12GraphicsCommandList& commandList,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept
{
	GeometryGenerator::MeshData meshData;
	GeometryGenerator::CreateSphere(radius, sliceCount, stackCount, meshData);

	mMutex.lock();
	model = new Model(meshData, commandList, uploadVertexBuffer, uploadIndexBuffer);
	mMutex.unlock();

	const std::size_t id{ NumberGeneration::GetIncrementalSizeT() };
	ModelById::accessor accessor;
#ifdef _DEBUG
	mModelById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mModelById.insert(accessor, id);
	accessor->second.reset(model);
	accessor.release();

	return id;
}

std::size_t ModelManager::CreateGeosphere(
	const float radius, 
	const std::uint32_t numSubdivisions, 
	Model* &model, 
	ID3D12GraphicsCommandList& commandList,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept 
{
	GeometryGenerator::MeshData meshData;
	GeometryGenerator::CreateGeosphere(radius, numSubdivisions, meshData);

	mMutex.lock();
	model = new Model(meshData, commandList, uploadVertexBuffer, uploadIndexBuffer);
	mMutex.unlock();

	const std::size_t id{ NumberGeneration::GetIncrementalSizeT() };
	ModelById::accessor accessor;
#ifdef _DEBUG
	mModelById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mModelById.insert(accessor, id);
	accessor->second.reset(model);
	accessor.release();

	return id;
}

std::size_t ModelManager::CreateCylinder(
	const float bottomRadius,
	const float topRadius,
	const float height, 
	const std::uint32_t sliceCount,
	const std::uint32_t stackCount,
	Model* &model, 
	ID3D12GraphicsCommandList& commandList,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept 
{
	GeometryGenerator::MeshData meshData;
	GeometryGenerator::CreateCylinder(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);

	mMutex.lock();
	model = new Model(meshData, commandList, uploadVertexBuffer, uploadIndexBuffer);
	mMutex.unlock();

	const std::size_t id{ NumberGeneration::GetIncrementalSizeT() };
	ModelById::accessor accessor;
#ifdef _DEBUG
	mModelById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mModelById.insert(accessor, id);
	accessor->second.reset(model);
	accessor.release();

	return id;
}

std::size_t ModelManager::CreateGrid(
	const float width, 
	const float depth, 
	const std::uint32_t rows, 
	const std::uint32_t columns, 
	Model* &model, 
	ID3D12GraphicsCommandList& commandList,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept 
{
	GeometryGenerator::MeshData meshData;
	GeometryGenerator::CreateGrid(width, depth, rows, columns, meshData);

	mMutex.lock();
	model = new Model(meshData, commandList, uploadVertexBuffer, uploadIndexBuffer);
	mMutex.unlock();

	const std::size_t id{ NumberGeneration::GetIncrementalSizeT() };
	ModelById::accessor accessor;
#ifdef _DEBUG
	mModelById.find(accessor, id);
	ASSERT(accessor.empty());
#endif
	mModelById.insert(accessor, id);
	accessor->second.reset(model);
	accessor.release();

	return id;
}

Model& ModelManager::GetModel(const std::size_t id) noexcept {
	ModelById::accessor accessor;
	mModelById.find(accessor, id);
	ASSERT(!accessor.empty());

	Model* model{ accessor->second.get() };
	accessor.release();

	return *model;
}
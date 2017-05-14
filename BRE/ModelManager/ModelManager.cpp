#include "ModelManager.h"

#include <GeometryGenerator\GeometryGenerator.h>
#include <Utils/DebugUtils.h>

namespace BRE {
tbb::concurrent_unordered_set<Model*> ModelManager::mModels;
std::mutex ModelManager::mMutex;

void
ModelManager::Clear() noexcept
{
    for (Model* model : mModels) {
        BRE_ASSERT(model != nullptr);
        delete model;
    }

    mModels.clear();
}

Model&
ModelManager::LoadModel(const char* modelFilename,
                        ID3D12GraphicsCommandList& commandList,
                        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept
{
    BRE_ASSERT(modelFilename != nullptr);

    Model* model{ nullptr };

    mMutex.lock();
    model = new Model(modelFilename,
                      commandList,
                      uploadVertexBuffer,
                      uploadIndexBuffer);
    mMutex.unlock();

    BRE_ASSERT(model != nullptr);
    mModels.insert(model);

    return *model;
}

Model&
ModelManager::CreateBox(const float width,
                        const float height,
                        const float depth,
                        const std::uint32_t numSubdivisions,
                        ID3D12GraphicsCommandList& commandList,
                        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept
{
    Model* model{ nullptr };

    GeometryGenerator::MeshData meshData;
    GeometryGenerator::CreateBox(width,
                                 height,
                                 depth,
                                 numSubdivisions, meshData);

    mMutex.lock();
    model = new Model(meshData,
                      commandList,
                      uploadVertexBuffer,
                      uploadIndexBuffer);
    mMutex.unlock();

    BRE_ASSERT(model != nullptr);
    mModels.insert(model);

    return *model;
}

Model&
ModelManager::CreateSphere(const float radius,
                           const std::uint32_t sliceCount,
                           const std::uint32_t stackCount,
                           ID3D12GraphicsCommandList& commandList,
                           Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                           Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept
{
    Model* model{ nullptr };

    GeometryGenerator::MeshData meshData;
    GeometryGenerator::CreateSphere(radius,
                                    sliceCount,
                                    stackCount,
                                    meshData);

    mMutex.lock();
    model = new Model(meshData,
                      commandList,
                      uploadVertexBuffer,
                      uploadIndexBuffer);
    mMutex.unlock();

    BRE_ASSERT(model != nullptr);
    mModels.insert(model);

    return *model;
}

Model&
ModelManager::CreateGeosphere(const float radius,
                              const std::uint32_t numSubdivisions,
                              ID3D12GraphicsCommandList& commandList,
                              Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                              Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept
{
    Model* model{ nullptr };

    GeometryGenerator::MeshData meshData;
    GeometryGenerator::CreateGeosphere(radius,
                                       numSubdivisions,
                                       meshData);

    mMutex.lock();
    model = new Model(meshData,
                      commandList,
                      uploadVertexBuffer,
                      uploadIndexBuffer);
    mMutex.unlock();

    BRE_ASSERT(model != nullptr);
    mModels.insert(model);

    return *model;
}

Model&
ModelManager::CreateCylinder(const float bottomRadius,
                             const float topRadius,
                             const float height,
                             const std::uint32_t sliceCount,
                             const std::uint32_t stackCount,
                             ID3D12GraphicsCommandList& commandList,
                             Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                             Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept
{
    Model* model{ nullptr };

    GeometryGenerator::MeshData meshData;
    GeometryGenerator::CreateCylinder(bottomRadius,
                                      topRadius,
                                      height,
                                      sliceCount,
                                      stackCount,
                                      meshData);

    mMutex.lock();
    model = new Model(meshData,
                      commandList,
                      uploadVertexBuffer,
                      uploadIndexBuffer);
    mMutex.unlock();

    BRE_ASSERT(model != nullptr);
    mModels.insert(model);

    return *model;
}

Model&
ModelManager::CreateGrid(const float width,
                         const float depth,
                         const std::uint32_t rows,
                         const std::uint32_t columns,
                         ID3D12GraphicsCommandList& commandList,
                         Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                         Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept
{
    Model* model{ nullptr };

    GeometryGenerator::MeshData meshData;
    GeometryGenerator::CreateGrid(width,
                                  depth,
                                  rows,
                                  columns,
                                  meshData);

    mMutex.lock();
    model = new Model(meshData,
                      commandList,
                      uploadVertexBuffer,
                      uploadIndexBuffer);
    mMutex.unlock();

    BRE_ASSERT(model != nullptr);
    mModels.insert(model);

    return *model;
}
}


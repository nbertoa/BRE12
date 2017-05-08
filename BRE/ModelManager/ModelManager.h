#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb\concurrent_unordered_set.h>

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

    static void EraseAll() noexcept;

    static Model& LoadModel(const char* modelFilename,
                            ID3D12GraphicsCommandList& commandList,
                            Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                            Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

    // Geometry is centered at the origin.
    static Model& CreateBox(const float width,
                            const float height,
                            const float depth,
                            const std::uint32_t numSubdivisions,
                            ID3D12GraphicsCommandList& commandList,
                            Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                            Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

    // Geometry is centered at the origin.
    static Model& CreateSphere(const float radius,
                               const std::uint32_t sliceCount,
                               const std::uint32_t stackCount,
                               ID3D12GraphicsCommandList& commandList,
                               Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                               Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

    // Geometry is centered at the origin.
    static Model& CreateGeosphere(const float radius,
                                  const std::uint32_t numSubdivisions,
                                  ID3D12GraphicsCommandList& commandList,
                                  Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                                  Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

    // Creates a cylinder parallel to the y-axis, and centered about the origin.  
    static Model& CreateCylinder(const float bottomRadius,
                                 const float topRadius,
                                 const float height,
                                 const std::uint32_t sliceCount,
                                 const std::uint32_t stackCount,
                                 ID3D12GraphicsCommandList& commandList,
                                 Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                                 Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

    // Creates a rows x columns grid in the xz-plane centered
    // at the origin.
    static Model& CreateGrid(const float width,
                             const float depth,
                             const std::uint32_t rows,
                             const std::uint32_t columns,
                             ID3D12GraphicsCommandList& commandList,
                             Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                             Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept;

private:
    using Models = tbb::concurrent_unordered_set<Model*>;
    static Models mModels;

    static std::mutex mMutex;
};

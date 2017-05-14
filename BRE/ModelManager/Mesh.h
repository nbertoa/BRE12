#pragma once

#include <cstdint>

#include <GeometryGenerator/GeometryGenerator.h>
#include <ResourceManager\VertexAndIndexBufferCreator.h>
#include <Utils/DebugUtils.h>

struct aiMesh;

namespace BRE {
class Model;

///
/// @brief Stores model's mesh vertex and index data.
///
class Mesh {
    friend class Model;

public:
    ~Mesh() = default;
    Mesh(const Mesh&) = delete;
    const Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) = default;
    Mesh& operator=(Mesh&&) = delete;

    ///
    /// @brief Get vertex buffer data
    ///
    /// Data must be valid
    ///
    /// @return Vertex buffer data
    ///
    __forceinline const VertexAndIndexBufferCreator::VertexBufferData& GetVertexBufferData() const noexcept
    {
        BRE_ASSERT(mVertexBufferData.IsDataValid());
        return mVertexBufferData;
    }

    ///
    /// @brief Get index buffer data
    ///
    /// Data must be valid
    ///
    /// @return Index buffer data
    ///
    __forceinline const VertexAndIndexBufferCreator::IndexBufferData& GetIndexBufferData() const noexcept
    {
        BRE_ASSERT(mIndexBufferData.IsDataValid());
        return mIndexBufferData;
    }

private:
    ///
    /// @brief Mesh constructor
    /// @param mesh Assimp mesh
    /// @param commandList Command list to create the vertex and index buffer for the mesh
    /// @param uploadVertexBuffer Upload vertex buffer
    /// @param uploadIndexBuffer Upload index buffer
    ///
    explicit Mesh(const aiMesh& mesh,
                  ID3D12GraphicsCommandList& commandList,
                  Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                  Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer);

    ///
    /// @brief Mesh constructor
    /// @param meshData Mesh data where we extract vertex and indices
    /// @param commandList Command list to create the vertex and index buffer
    /// @param uploadVertexBuffer Upload vertex buffer
    /// @param uploadIndexBuffer Upload index buffer
    ///
    explicit Mesh(const GeometryGenerator::MeshData& meshData,
                  ID3D12GraphicsCommandList& commandList,
                  Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                  Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer);

    VertexAndIndexBufferCreator::VertexBufferData mVertexBufferData;
    VertexAndIndexBufferCreator::IndexBufferData mIndexBufferData;
};
}


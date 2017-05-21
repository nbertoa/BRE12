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
    /// @param commandList Command list used to upload buffers content to GPU.
    /// It must be executed after this function call to upload buffers content to GPU.
    /// It must be in recording state before calling this method.
    /// @param uploadVertexBuffer Upload buffer to upload the vertex buffer content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadVertexBuffer after it knows the copy has been executed.
    /// @param uploadIndexBuffer Upload buffer to upload the index buffer content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadIndexBuffer after it knows the copy has been executed.
    ///
    explicit Mesh(const aiMesh& mesh,
                  ID3D12GraphicsCommandList& commandList,
                  Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                  Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer);

    ///
    /// @brief Mesh constructor
    /// @param meshData Mesh data where we extract vertex and indices
    /// @param commandList Command list used to upload buffers content to GPU.
    /// It must be executed after this function call to upload buffers content to GPU.
    /// It must be in recording state before calling this method.
    /// @param uploadVertexBuffer Upload buffer to upload the vertex buffer content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadVertexBuffer after it knows the copy has been executed.
    /// @param uploadIndexBuffer Upload buffer to upload the index buffer content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadIndexBuffer after it knows the copy has been executed.
    ///
    explicit Mesh(const GeometryGenerator::MeshData& meshData,
                  ID3D12GraphicsCommandList& commandList,
                  Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                  Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer);

    VertexAndIndexBufferCreator::VertexBufferData mVertexBufferData;
    VertexAndIndexBufferCreator::IndexBufferData mIndexBufferData;
};
}
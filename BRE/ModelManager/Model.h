#pragma once

#include <vector>

#include <GeometryGenerator/GeometryGenerator.h>
#include <ModelManager/Mesh.h>

namespace BRE {
///
/// @brief Represents a model that can be loaded from a file
///
class Model {
public:
    ~Model() = default;
    Model(const Model&) = delete;
    const Model& operator=(const Model&) = delete;
    Model(Model&&) = delete;
    Model& operator=(Model&&) = delete;

    ///
    /// @brief Model constructor
    /// @param modelFilename Model filename. Must not be nullptr.
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
    explicit Model(const char* modelFilename,
                   ID3D12GraphicsCommandList& commandList,
                   ID3D12Resource* &ploadVertexBuffer,
                   ID3D12Resource* &uploadIndexBuffer);

    ///
    /// @brief Model constructor
    /// @param meshData Mesh data.
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
    explicit Model(const GeometryGenerator::MeshData& meshData,
                   ID3D12GraphicsCommandList& commandList,
                   ID3D12Resource* &uploadVertexBuffer,
                   ID3D12Resource* &uploadIndexBuffer);

    ///
    /// @brief Checks if there are meshes or not
    /// @return True if there are meshes. Otherwise, false.
    ///
    __forceinline bool HasMeshes() const noexcept
    {
        return (mMeshes.size() > 0UL);
    }

    ///
    /// @brief Get meshes
    /// @return List of meshes
    ///
    __forceinline const std::vector<Mesh>& GetMeshes() const noexcept
    {
        return mMeshes;
    }

private:
    std::vector<Mesh> mMeshes;
};
}
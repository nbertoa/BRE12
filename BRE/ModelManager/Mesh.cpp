#include "Mesh.h"

#include <assimp/scene.h>

#include <Utils/DebugUtils.h>

using namespace DirectX;

namespace BRE {
namespace {
///
/// @brief Creates vertex and index buffer data
/// @param vertexBufferData Vertex buffer data
/// @param indexBufferData Index buffer data
/// @param meshData Mesh data to get vertices and indices
/// @param commandList Command list to create the buffers
/// @param uploadVertexBuffer Upload vertex buffer
/// @param uploadIndexBuffer Upload index buffer
///
void CreateVertexAndIndexBufferData(VertexAndIndexBufferCreator::VertexBufferData& vertexBufferData,
                                    VertexAndIndexBufferCreator::IndexBufferData& indexBufferData,
                                    const GeometryGenerator::MeshData& meshData,
                                    ID3D12GraphicsCommandList& commandList,
                                    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
                                    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer) noexcept
{
    BRE_ASSERT(vertexBufferData.IsDataValid() == false);
    BRE_ASSERT(indexBufferData.IsDataValid() == false);

    // Create vertex buffer
    VertexAndIndexBufferCreator::BufferCreationData vertexBufferParams(meshData.mVertices.data(),
                                                                       static_cast<std::uint32_t>(meshData.mVertices.size()),
                                                                       sizeof(GeometryGenerator::Vertex));

    VertexAndIndexBufferCreator::CreateVertexBuffer(commandList,
                                                    vertexBufferParams,
                                                    vertexBufferData,
                                                    uploadVertexBuffer);

    // Create index buffer
    VertexAndIndexBufferCreator::BufferCreationData indexBufferParams(meshData.mIndices32.data(),
                                                                      static_cast<std::uint32_t>(meshData.mIndices32.size()),
                                                                      sizeof(std::uint32_t));

    VertexAndIndexBufferCreator::CreateIndexBuffer(commandList,
                                                   indexBufferParams,
                                                   indexBufferData,
                                                   uploadIndexBuffer);

    BRE_ASSERT(vertexBufferData.IsDataValid());
    BRE_ASSERT(indexBufferData.IsDataValid());
}
}

Mesh::Mesh(const aiMesh& mesh,
           ID3D12GraphicsCommandList& commandList,
           Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
           Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer)
{
    GeometryGenerator::MeshData meshData;

    // Positions and Normals
    const std::size_t numVertices{ mesh.mNumVertices };
    BRE_ASSERT(numVertices > 0U);
    BRE_ASSERT(mesh.HasNormals());
    meshData.mVertices.resize(numVertices);
    for (std::uint32_t i = 0U; i < numVertices; ++i) {
        meshData.mVertices[i].mPosition = XMFLOAT3(reinterpret_cast<const float*>(&mesh.mVertices[i]));
        meshData.mVertices[i].mNormal = XMFLOAT3(reinterpret_cast<const float*>(&mesh.mNormals[i]));
    }

    // Texture Coordinates (if any)
    if (mesh.HasTextureCoords(0U)) {
        BRE_ASSERT(mesh.GetNumUVChannels() == 1U);
        const aiVector3D* aiTextureCoordinates{ mesh.mTextureCoords[0U] };
        BRE_ASSERT(aiTextureCoordinates != nullptr);
        for (std::uint32_t i = 0U; i < numVertices; i++) {
            meshData.mVertices[i].mUV = XMFLOAT2(reinterpret_cast<const float*>(&aiTextureCoordinates[i]));
        }
    }

    // Indices
    BRE_ASSERT(mesh.HasFaces());
    const std::uint32_t numFaces{ mesh.mNumFaces };
    for (std::uint32_t i = 0U; i < numFaces; ++i) {
        const aiFace* face = &mesh.mFaces[i];
        BRE_ASSERT(face != nullptr);
        // We only allow triangles
        BRE_ASSERT(face->mNumIndices == 3U);

        meshData.mIndices32.push_back(face->mIndices[0U]);
        meshData.mIndices32.push_back(face->mIndices[1U]);
        meshData.mIndices32.push_back(face->mIndices[2U]);
    }

    // Tangents
    if (mesh.HasTangentsAndBitangents()) {
        for (std::uint32_t i = 0U; i < numVertices; ++i) {
            meshData.mVertices[i].mTangent = XMFLOAT3(reinterpret_cast<const float*>(&mesh.mTangents[i]));
        }
    }

    CreateVertexAndIndexBufferData(mVertexBufferData,
                                   mIndexBufferData,
                                   meshData,
                                   commandList,
                                   uploadVertexBuffer,
                                   uploadIndexBuffer);

    BRE_ASSERT(mVertexBufferData.IsDataValid());
    BRE_ASSERT(mIndexBufferData.IsDataValid());
}

Mesh::Mesh(const GeometryGenerator::MeshData& meshData,
           ID3D12GraphicsCommandList& commandList,
           Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
           Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer)
{
    CreateVertexAndIndexBufferData(mVertexBufferData,
                                   mIndexBufferData,
                                   meshData,
                                   commandList,
                                   uploadVertexBuffer,
                                   uploadIndexBuffer);

    BRE_ASSERT(mVertexBufferData.IsDataValid());
    BRE_ASSERT(mIndexBufferData.IsDataValid());
}
}


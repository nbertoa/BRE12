#include "Model.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <ResourceManager/ResourceManager.h>
#include <SettingsManager\SettingsManager.h>
#include <Utils/DebugUtils.h>

namespace BRE {
Model::Model(const char* modelFilename,
             ID3D12GraphicsCommandList& commandList,
             Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
             Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer)
{
    BRE_ASSERT(modelFilename != nullptr);
    const std::string filePath(modelFilename);

    Assimp::Importer importer;
    const std::uint32_t flags{ aiProcessPreset_TargetRealtime_Fast | aiProcess_ConvertToLeftHanded };
    const aiScene* scene{ importer.ReadFile(filePath.c_str(), flags) };
    if (scene == nullptr) {
        const std::string errorMessage{ importer.GetErrorString() };
        const std::wstring wideErrorMessage = StringUtils::AnsiToWideString(errorMessage);
        MessageBox(nullptr, wideErrorMessage.c_str(), nullptr, 0);
        BRE_ASSERT(scene != nullptr);
    }

    BRE_ASSERT(scene->HasMeshes());

    for (std::uint32_t i = 0U; i < scene->mNumMeshes; ++i) {
        aiMesh* mesh{ scene->mMeshes[i] };
        BRE_ASSERT(mesh != nullptr);
        mMeshes.push_back(Mesh(*mesh,
                               commandList,
                               uploadVertexBuffer,
                               uploadIndexBuffer));
    }
}

Model::Model(const GeometryGenerator::MeshData& meshData,
             ID3D12GraphicsCommandList& commandList,
             Microsoft::WRL::ComPtr<ID3D12Resource>& uploadVertexBuffer,
             Microsoft::WRL::ComPtr<ID3D12Resource>& uploadIndexBuffer)
{
    mMeshes.push_back(Mesh(meshData,
                           commandList,
                           uploadVertexBuffer,
                           uploadIndexBuffer));
}
}


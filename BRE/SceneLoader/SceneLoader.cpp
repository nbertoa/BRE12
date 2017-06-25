#include "SceneLoader.h"

#include <cstdint>
#include <d3d12.h>
#include <string>
#pragma warning( push )
#pragma warning( disable : 4127)
#include <yaml-cpp/yaml.h>
#pragma warning( pop ) 

#include <CommandManager\CommandAllocatorManager.h>
#include <CommandManager\CommandListManager.h>
#include <GeometryPass\Recorders\HeightMappingCommandListRecorder.h>
#include <GeometryPass\Recorders\NormalMappingCommandListRecorder.h>
#include <GeometryPass\Recorders\TextureMappingCommandListRecorder.h>
#include <MathUtils\MathUtils.h>
#include <ModelManager\Model.h>
#include <Scene\Scene.h>
#include <Utils/DebugUtils.h>

using namespace DirectX;

namespace BRE {
SceneLoader::SceneLoader()
    : mMaterialTechniqueLoader(mTextureLoader)
    , mDrawableObjectLoader(mMaterialTechniqueLoader, mModelLoader)
    , mEnvironmentLoader(mTextureLoader)
{

    mCommandAllocator = &CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
    mCommandList = &CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *mCommandAllocator);
    mCommandList->Close();
};

Scene*
SceneLoader::LoadScene(const char* sceneFilePath) noexcept
{
    BRE_ASSERT(sceneFilePath != nullptr);

    const YAML::Node rootNode = YAML::LoadFile(sceneFilePath);
    const std::wstring errorMsg =
        L"Failed to open yaml file: " + StringUtils::AnsiToWideString(sceneFilePath);
    BRE_CHECK_MSG(rootNode.IsDefined(), errorMsg.c_str());

    mModelLoader.LoadModels(rootNode, *mCommandAllocator, *mCommandList);
    mTextureLoader.LoadTextures(rootNode, *mCommandAllocator, *mCommandList);
    mMaterialTechniqueLoader.LoadMaterialTechniques(rootNode);
    mDrawableObjectLoader.LoadDrawableObjects(rootNode);
    mEnvironmentLoader.LoadEnvironment(rootNode);
    mCameraLoader.LoadCamera(rootNode);

    Scene* scene = new Scene;
    GenerateGeometryPassRecorders(*scene);
    scene->GetCamera() = mCameraLoader.GetCamera();

    return scene;
}

void
SceneLoader::GenerateGeometryPassRecorders(Scene& scene) noexcept
{
    GenerateGeometryPassRecordersForTextureMapping(scene.GetGeometryCommandListRecorders());
    GenerateGeometryPassRecordersForNormalMapping(scene.GetGeometryCommandListRecorders());
    GenerateGeometryPassRecordersForHeightMapping(scene.GetGeometryCommandListRecorders());

    scene.GetSkyBoxCubeMap() = &mEnvironmentLoader.GetSkyBoxTexture();
    scene.GetDiffuseIrradianceCubeMap() = &mEnvironmentLoader.GetDiffuseIrradianceTexture();
    scene.GetSpecularPreConvolvedCubeMap() = &mEnvironmentLoader.GetSpecularPreConvolvedEnvironmentTexture();
}

void
SceneLoader::GenerateGeometryPassRecordersForTextureMapping(GeometryCommandListRecorders& commandListRecorders) noexcept
{
    const DrawableObjectLoader::DrawableObjectsByModelName& drawableObjectsByModelName =
        mDrawableObjectLoader.GetDrawableObjectsByModelNameByTechniqueType(MaterialTechnique::TEXTURE_MAPPING);

    if (drawableObjectsByModelName.empty()) {
        return;
    }

    // Iterate over Drawable objects and fill containers needed
    // to initialize the command list recorder.
    TextureMappingCommandListRecorder* commandListRecorder = new TextureMappingCommandListRecorder;
    std::vector<GeometryCommandListRecorder::GeometryData> geometryDataVector;
    std::vector<ID3D12Resource*> baseColorTextures;
    std::vector<ID3D12Resource*> metalnessTextures;
    std::vector<ID3D12Resource*> roughnessTextures;

    std::size_t geometryDataVectorOffset = 0;
    for (const DrawableObjectLoader::DrawableObjectsByModelName::value_type& pair : drawableObjectsByModelName) {
        const std::vector<DrawableObject>& drawableObjects = pair.second;
        BRE_ASSERT(drawableObjects.empty() == false);

        // Build geometry data vertex and index buffers for all meshes
        const Model& model = drawableObjects[0].GetModel();
        const std::vector<Mesh>& meshes = model.GetMeshes();
        const std::size_t totalDataCount = meshes.size() * drawableObjects.size();
        geometryDataVector.reserve(geometryDataVector.size() + totalDataCount);
        for (std::uint32_t i = 0U; i < meshes.size(); ++i) {
            const Mesh& mesh = meshes[i];
            GeometryCommandListRecorder::GeometryData geometryData;
            geometryData.mVertexBufferData = mesh.GetVertexBufferData();
            geometryData.mIndexBufferData = mesh.GetIndexBufferData();
            geometryData.mWorldMatrices.reserve(drawableObjects.size());
            geometryData.mInverseTransposeWorldMatrices.reserve(drawableObjects.size());
            geometryData.mTextureScales.reserve(drawableObjects.size());
            geometryDataVector.emplace_back(geometryData);
        }

        // Iterate all the meses and store data for all the drawable objects.
        for (std::uint32_t i = 0U; i < meshes.size(); ++i) {
            GeometryCommandListRecorder::GeometryData& geometryData =
                geometryDataVector[geometryDataVectorOffset + i];

            for (const DrawableObject& drawableObject : drawableObjects) {
                // Store textures
                const MaterialTechnique& materialTechnique = drawableObject.GetMaterialTechnique();
                baseColorTextures.push_back(&materialTechnique.GetBaseColorTexture());
                metalnessTextures.push_back(&materialTechnique.GetMetalnessTexture());
                roughnessTextures.push_back(&materialTechnique.GetRoughnessTexture());

                // Store matrices and texture scale
                const XMFLOAT4X4& worldMatrix = drawableObject.GetWorldMatrix();
                XMFLOAT4X4 inverseTransposeWorldMatrix;
                MathUtils::StoreInverseTransposeMatrix(worldMatrix, inverseTransposeWorldMatrix);
                geometryData.mWorldMatrices.push_back(worldMatrix);
                geometryData.mInverseTransposeWorldMatrices.push_back(inverseTransposeWorldMatrix);
                geometryData.mTextureScales.push_back(drawableObject.GetTextureScale());
            }
        }

        geometryDataVectorOffset += meshes.size();
    }

    commandListRecorder->Init(geometryDataVector,
                              baseColorTextures,
                              metalnessTextures,
                              roughnessTextures);

    commandListRecorders.push_back(std::unique_ptr<GeometryCommandListRecorder>(commandListRecorder));
}

void
SceneLoader::GenerateGeometryPassRecordersForNormalMapping(GeometryCommandListRecorders& commandListRecorders) noexcept
{
    const DrawableObjectLoader::DrawableObjectsByModelName& drawableObjectsByModelName =
        mDrawableObjectLoader.GetDrawableObjectsByModelNameByTechniqueType(MaterialTechnique::NORMAL_MAPPING);

    if (drawableObjectsByModelName.empty()) {
        return;
    }

    // Iterate over Drawable objects and fill containers needed
    // to initialize the command list recorder.
    NormalMappingCommandListRecorder* commandListRecorder = new NormalMappingCommandListRecorder;
    std::vector<GeometryCommandListRecorder::GeometryData> geometryDataVector;
    std::vector<ID3D12Resource*> baseColorTextures;
    std::vector<ID3D12Resource*> metalnessTextures;
    std::vector<ID3D12Resource*> roughnessTextures;
    std::vector<ID3D12Resource*> normalTextures;

    std::size_t geometryDataVectorOffset = 0;
    for (const DrawableObjectLoader::DrawableObjectsByModelName::value_type& pair : drawableObjectsByModelName) {
        const std::vector<DrawableObject>& drawableObjects = pair.second;
        BRE_ASSERT(drawableObjects.empty() == false);

        // Build geometry data vertex and index buffers for all meshes
        const Model& model = drawableObjects[0].GetModel();
        const std::vector<Mesh>& meshes = model.GetMeshes();
        const std::size_t totalDataCount = meshes.size() * drawableObjects.size();
        geometryDataVector.reserve(geometryDataVector.size() + totalDataCount);
        for (std::uint32_t i = 0U; i < meshes.size(); ++i) {
            const Mesh& mesh = meshes[i];
            GeometryCommandListRecorder::GeometryData geometryData;
            geometryData.mVertexBufferData = mesh.GetVertexBufferData();
            geometryData.mIndexBufferData = mesh.GetIndexBufferData();
            geometryData.mWorldMatrices.reserve(drawableObjects.size());
            geometryData.mInverseTransposeWorldMatrices.reserve(drawableObjects.size());
            geometryData.mTextureScales.reserve(drawableObjects.size());
            geometryDataVector.emplace_back(geometryData);
        }

        // Iterate all the meses and store data for all the drawable objects.
        for (std::uint32_t i = 0U; i < meshes.size(); ++i) {
            GeometryCommandListRecorder::GeometryData& geometryData =
                geometryDataVector[geometryDataVectorOffset + i];

            for (const DrawableObject& drawableObject : drawableObjects) {
                // Store textures
                const MaterialTechnique& materialTechnique = drawableObject.GetMaterialTechnique();
                baseColorTextures.push_back(&materialTechnique.GetBaseColorTexture());
                metalnessTextures.push_back(&materialTechnique.GetMetalnessTexture());
                roughnessTextures.push_back(&materialTechnique.GetRoughnessTexture());
                normalTextures.push_back(&materialTechnique.GetNormalTexture());

                // Store matrices and texture scale
                const XMFLOAT4X4& worldMatrix = drawableObject.GetWorldMatrix();
                XMFLOAT4X4 inverseTransposeWorldMatrix;
                MathUtils::StoreInverseTransposeMatrix(worldMatrix, inverseTransposeWorldMatrix);
                geometryData.mWorldMatrices.push_back(worldMatrix);
                geometryData.mInverseTransposeWorldMatrices.push_back(inverseTransposeWorldMatrix);
                geometryData.mTextureScales.push_back(drawableObject.GetTextureScale());
            }
        }

        geometryDataVectorOffset += meshes.size();
    }

    commandListRecorder->Init(geometryDataVector,
                              baseColorTextures,
                              metalnessTextures,
                              roughnessTextures,
                              normalTextures);

    commandListRecorders.push_back(std::unique_ptr<GeometryCommandListRecorder>(commandListRecorder));
}

void
SceneLoader::GenerateGeometryPassRecordersForHeightMapping(GeometryCommandListRecorders& commandListRecorders) noexcept
{
    const DrawableObjectLoader::DrawableObjectsByModelName& drawableObjectsByModelName =
        mDrawableObjectLoader.GetDrawableObjectsByModelNameByTechniqueType(MaterialTechnique::HEIGHT_MAPPING);

    if (drawableObjectsByModelName.empty()) {
        return;
    }

    // Iterate over Drawable objects and fill containers needed
    // to initialize the command list recorder.
    HeightMappingCommandListRecorder* commandListRecorder = new HeightMappingCommandListRecorder;
    std::vector<GeometryCommandListRecorder::GeometryData> geometryDataVector;
    std::vector<ID3D12Resource*> baseColorTextures;
    std::vector<ID3D12Resource*> metalnessTextures;
    std::vector<ID3D12Resource*> roughnessTextures;
    std::vector<ID3D12Resource*> normalTextures;
    std::vector<ID3D12Resource*> heightTextures;

    std::size_t geometryDataVectorOffset = 0;
    for (const DrawableObjectLoader::DrawableObjectsByModelName::value_type& pair : drawableObjectsByModelName) {
        const std::vector<DrawableObject>& drawableObjects = pair.second;
        BRE_ASSERT(drawableObjects.empty() == false);

        // Build geometry data vertex and index buffers for all meshes
        const Model& model = drawableObjects[0].GetModel();
        const std::vector<Mesh>& meshes = model.GetMeshes();
        const std::size_t totalDataCount = meshes.size() * drawableObjects.size();
        geometryDataVector.reserve(geometryDataVector.size() + totalDataCount);
        for (std::uint32_t i = 0U; i < meshes.size(); ++i) {
            const Mesh& mesh = meshes[i];
            GeometryCommandListRecorder::GeometryData geometryData;
            geometryData.mVertexBufferData = mesh.GetVertexBufferData();
            geometryData.mIndexBufferData = mesh.GetIndexBufferData();
            geometryData.mWorldMatrices.reserve(drawableObjects.size());
            geometryData.mInverseTransposeWorldMatrices.reserve(drawableObjects.size());
            geometryData.mTextureScales.reserve(drawableObjects.size());
            geometryDataVector.emplace_back(geometryData);
        }

        // Iterate all the meses and store data for all the drawable objects.
        for (std::uint32_t i = 0U; i < meshes.size(); ++i) {
            GeometryCommandListRecorder::GeometryData& geometryData =
                geometryDataVector[geometryDataVectorOffset + i];

            for (const DrawableObject& drawableObject : drawableObjects) {
                // Store textures
                const MaterialTechnique& materialTechnique = drawableObject.GetMaterialTechnique();
                baseColorTextures.push_back(&materialTechnique.GetBaseColorTexture());
                metalnessTextures.push_back(&materialTechnique.GetMetalnessTexture());
                roughnessTextures.push_back(&materialTechnique.GetRoughnessTexture());
                normalTextures.push_back(&materialTechnique.GetNormalTexture());
                heightTextures.push_back(&materialTechnique.GetHeightTexture());

                // Store matrices and texture scale
                const XMFLOAT4X4& worldMatrix = drawableObject.GetWorldMatrix();
                XMFLOAT4X4 inverseTransposeWorldMatrix;
                MathUtils::StoreInverseTransposeMatrix(worldMatrix, inverseTransposeWorldMatrix);
                geometryData.mWorldMatrices.push_back(worldMatrix);
                geometryData.mInverseTransposeWorldMatrices.push_back(inverseTransposeWorldMatrix);
                geometryData.mTextureScales.push_back(drawableObject.GetTextureScale());
            }
        }

        geometryDataVectorOffset += meshes.size();
    }

    commandListRecorder->Init(geometryDataVector,
                              baseColorTextures,
                              metalnessTextures,
                              roughnessTextures,
                              normalTextures,
                              heightTextures);

    commandListRecorders.push_back(std::unique_ptr<GeometryCommandListRecorder>(commandListRecorder));
}
}
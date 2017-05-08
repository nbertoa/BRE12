#pragma once

#include <memory>

#include <GeometryPass\GeometryPassCmdListRecorder.h>
#include <SceneLoader\DrawableObjectLoader.h>
#include <SceneLoader\EnvironmentLoader.h>
#include <SceneLoader\MaterialPropertiesLoader.h>
#include <SceneLoader\MaterialTechniqueLoader.h>
#include <SceneLoader\ModelLoader.h>
#include <SceneLoader\TextureLoader.h>

struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;

namespace BRE {
class Scene;

class SceneLoader {
public:
    SceneLoader();
    SceneLoader(const SceneLoader&) = delete;
    const SceneLoader& operator=(const SceneLoader&) = delete;
    SceneLoader(SceneLoader&&) = delete;
    SceneLoader& operator=(SceneLoader&&) = delete;

    Scene* LoadScene(const char* sceneFilePath) noexcept;

private:
    void GenerateGeometryPassRecorders(Scene& scene) noexcept;
    void GenerateGeometryPassRecordersForColorMapping(GeometryPassCommandListRecorders& commandListRecorders) noexcept;
    void GenerateGeometryPassRecordersForColorNormalMapping(GeometryPassCommandListRecorders& commandListRecorders) noexcept;
    void GenerateGeometryPassRecordersForColorHeightMapping(GeometryPassCommandListRecorders& commandListRecorders) noexcept;
    void GenerateGeometryPassRecordersForTextureMapping(GeometryPassCommandListRecorders& commandListRecorders) noexcept;
    void GenerateGeometryPassRecordersForNormalMapping(GeometryPassCommandListRecorders& commandListRecorders) noexcept;
    void GenerateGeometryPassRecordersForHeightMapping(GeometryPassCommandListRecorders& commandListRecorders) noexcept;

    ID3D12CommandAllocator* mCommandAllocator{ nullptr };
    ID3D12GraphicsCommandList* mCommandList{ nullptr };
    ModelLoader mModelLoader;
    TextureLoader mTextureLoader;
    MaterialPropertiesLoader mMaterialPropertiesLoader;
    MaterialTechniqueLoader mMaterialTechniqueLoader;
    DrawableObjectLoader mDrawableObjectLoader;
    EnvironmentLoader mEnvironmentLoader;
};
}


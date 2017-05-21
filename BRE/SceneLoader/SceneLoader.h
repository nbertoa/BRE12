#pragma once

#include <memory>

#include <GeometryPass\GeometryCommandListRecorder.h>
#include <SceneLoader\CameraLoader.h>
#include <SceneLoader\DrawableObjectLoader.h>
#include <SceneLoader\EnvironmentLoader.h>
#include <SceneLoader\MaterialPropertiesLoader.h>
#include <SceneLoader\MaterialTechniqueLoader.h>
#include <SceneLoader\ModelLoader.h>
#include <SceneLoader\TextureLoader.h>


namespace BRE {
class Scene;

///
/// @brief Responsible to load scene file
///
class SceneLoader {
public:
    SceneLoader();
    SceneLoader(const SceneLoader&) = delete;
    const SceneLoader& operator=(const SceneLoader&) = delete;
    SceneLoader(SceneLoader&&) = delete;
    SceneLoader& operator=(SceneLoader&&) = delete;

    ///
    /// @brief Load scene
    /// @param rootNode Scene YAML file root node
    ///
    Scene* LoadScene(const char* sceneFilePath) noexcept;

private:
    ///
    /// @brief Generate geometry pass recorders
    /// @param scene Scene to initialize
    ///
    void GenerateGeometryPassRecorders(Scene& scene) noexcept;

    ///
    /// @brief Generate geometry pass command list recorders for color mapping
    /// @param commandListRecorders Geometry pass command list recorders
    ///
    void GenerateGeometryPassRecordersForColorMapping(GeometryCommandListRecorders& commandListRecorders) noexcept;

    ///
    /// @brief Generate geometry pass command list recorders for color normal mapping
    /// @param commandListRecorders Geometry pass command list recorders
    ///
    void GenerateGeometryPassRecordersForColorNormalMapping(GeometryCommandListRecorders& commandListRecorders) noexcept;

    ///
    /// @brief Generate geometry pass command list recorders for color height mapping
    /// @param commandListRecorders Geometry pass command list recorders
    ///
    void GenerateGeometryPassRecordersForColorHeightMapping(GeometryCommandListRecorders& commandListRecorders) noexcept;

    ///
    /// @brief Generate geometry pass command list recorders for texture mapping
    /// @param commandListRecorders Geometry pass command list recorders
    ///
    void GenerateGeometryPassRecordersForTextureMapping(GeometryCommandListRecorders& commandListRecorders) noexcept;

    ///
    /// @brief Generate geometry pass command list recorders for normal mapping
    /// @param commandListRecorders Geometry pass command list recorders
    ///
    void GenerateGeometryPassRecordersForNormalMapping(GeometryCommandListRecorders& commandListRecorders) noexcept;

    ///
    /// @brief Generate geometry pass command list recorders for height mapping
    /// @param commandListRecorders Geometry pass command list recorders
    ///
    void GenerateGeometryPassRecordersForHeightMapping(GeometryCommandListRecorders& commandListRecorders) noexcept;

    ID3D12CommandAllocator* mCommandAllocator{ nullptr };
    ID3D12GraphicsCommandList* mCommandList{ nullptr };
    ModelLoader mModelLoader;
    TextureLoader mTextureLoader;
    MaterialPropertiesLoader mMaterialPropertiesLoader;
    MaterialTechniqueLoader mMaterialTechniqueLoader;
    DrawableObjectLoader mDrawableObjectLoader;
    EnvironmentLoader mEnvironmentLoader;
    CameraLoader mCameraLoader;
};
}
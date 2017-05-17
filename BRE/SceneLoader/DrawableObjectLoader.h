#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <SceneLoader\DrawableObject.h>
#include <SceneLoader\MaterialTechnique.h>

namespace YAML {
class Node;
}

namespace BRE {
class MaterialPropertiesLoader;
class MaterialTechniqueLoader;
class ModelLoader;

///
/// @brief Responsible to load from scene file the objects configurations
///
class DrawableObjectLoader {
public:
    using DrawableObjectsByModelName = std::unordered_map<std::string, std::vector<DrawableObject>>;

    DrawableObjectLoader(const MaterialPropertiesLoader& materialPropertiesLoader,
                         const MaterialTechniqueLoader& materialTechniqueLoader,
                         const ModelLoader& modelLoader)
        : mMaterialPropertiesLoader(materialPropertiesLoader)
        , mMaterialTechniqueLoader(materialTechniqueLoader)
        , mModelLoader(modelLoader)
    {}

    DrawableObjectLoader(const DrawableObjectLoader&) = delete;
    const DrawableObjectLoader& operator=(const DrawableObjectLoader&) = delete;
    DrawableObjectLoader(DrawableObjectLoader&&) = delete;
    DrawableObjectLoader& operator=(DrawableObjectLoader&&) = delete;

    ///
    /// @brief Load drawable objects
    /// @param rootNode Scene YAML file root node
    ///
    void LoadDrawableObjects(const YAML::Node& rootNode) noexcept;

    ///
    /// @brief Get drawable objects by model name by technique
    /// @return Drawable object by model name
    ///
    const DrawableObjectsByModelName& GetDrawableObjectsByModelNameByTechniqueType(
        const MaterialTechnique::TechniqueType techniqueType) const noexcept
    {
        return mDrawableObjectsByModelName[techniqueType];
    }

private:
    DrawableObjectsByModelName mDrawableObjectsByModelName[MaterialTechnique::NUM_TECHNIQUES];

    const MaterialPropertiesLoader& mMaterialPropertiesLoader;
    const MaterialTechniqueLoader& mMaterialTechniqueLoader;
    const ModelLoader& mModelLoader;
};
}
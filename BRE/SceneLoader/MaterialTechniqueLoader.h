#pragma once

#include <string>
#include <unordered_map>

#include <SceneLoader\MaterialTechnique.h>

namespace YAML {
class Node;
}

namespace BRE {
class TextureLoader;

///
/// @brief Responsible to load from scene file the material techniques
///
class MaterialTechniqueLoader {
public:
    MaterialTechniqueLoader(TextureLoader& textureLoader)
        : mTextureLoader(textureLoader)
    {}

    MaterialTechniqueLoader(const MaterialTechniqueLoader&) = delete;
    const MaterialTechniqueLoader& operator=(const MaterialTechniqueLoader&) = delete;
    MaterialTechniqueLoader(MaterialTechniqueLoader&&) = delete;
    MaterialTechniqueLoader& operator=(MaterialTechniqueLoader&&) = delete;

    ///
    /// @brief Load material techniques
    /// @param rootNode Scene YAML file root node
    ///
    void LoadMaterialTechniques(const YAML::Node& rootNode) noexcept;

    ///
    /// @brief Get material technique
    /// @return Material technique
    ///
    const MaterialTechnique& GetMaterialTechnique(const std::string& name) const noexcept;

private:
    ///
    /// @brief Update material technique
    /// @param materialTechniquePropertyName Material technique property name
    /// @param materialTechniqueTextureName Material technique texture name
    /// @param materialTechnique Output material technique
    ///
    void UpdateMaterialTechnique(const std::string& materialTechniquePropertyName,
                                 const std::string& materialTechniqueTextureName,
                                 MaterialTechnique& materialTechnique) const noexcept;

    std::unordered_map<std::string, MaterialTechnique> mMaterialTechniqueByName;
    TextureLoader& mTextureLoader;
};
}


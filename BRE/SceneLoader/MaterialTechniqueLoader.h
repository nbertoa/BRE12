#pragma once

#include <string>
#include <unordered_map>

#include <SceneLoader\MaterialTechnique.h>

namespace YAML {
class Node;
}

namespace BRE {
class TextureLoader;

class MaterialTechniqueLoader {
public:
    MaterialTechniqueLoader(TextureLoader& textureLoader)
        : mTextureLoader(textureLoader)
    {}

    MaterialTechniqueLoader(const MaterialTechniqueLoader&) = delete;
    const MaterialTechniqueLoader& operator=(const MaterialTechniqueLoader&) = delete;
    MaterialTechniqueLoader(MaterialTechniqueLoader&&) = delete;
    MaterialTechniqueLoader& operator=(MaterialTechniqueLoader&&) = delete;

    void LoadMaterialTechniques(const YAML::Node& rootNode) noexcept;

    const MaterialTechnique& GetMaterialTechnique(const std::string& name) const noexcept;

private:
    void UpdateMaterialTechnique(const std::string& materialTechniquePropertyName,
                                 const std::string& materialTechniqueTextureName,
                                 MaterialTechnique& materialTechnique) const noexcept;

    std::unordered_map<std::string, MaterialTechnique> mMaterialTechniqueByName;
    TextureLoader& mTextureLoader;
};
}


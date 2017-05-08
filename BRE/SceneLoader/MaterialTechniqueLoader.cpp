#include "MaterialTechniqueLoader.h"

#pragma warning( push )
#pragma warning( disable : 4127)
#include <yaml-cpp/yaml.h>
#pragma warning( pop ) 

#include <SceneLoader\TextureLoader.h>
#include <SceneLoader\YamlUtils.h>
#include <Utils/DebugUtils.h>

namespace BRE {
void
MaterialTechniqueLoader::LoadMaterialTechniques(const YAML::Node& rootNode) noexcept
{
    BRE_ASSERT(rootNode.IsDefined());

    // Get the "material techniques" node. It is a sequence of maps and its sintax is:
    // material techniques:
    //   - name: techniqueName1
    //     diffuse texture: diffuseTextureName
    //     normal texture: normalTextureName
    //     height texture: heightTextureName
    //   - name: techniqueName2
    //     diffuse texture: diffuseTextureName
    //     normal texture: normalTextureName
    const YAML::Node materialTechniquesNode = rootNode["material techniques"];
    BRE_ASSERT_MSG(materialTechniquesNode.IsDefined(), L"'material techniques' node is not defined");
    BRE_ASSERT_MSG(materialTechniquesNode.IsSequence(), L"'material techniques' node must be a map");

    std::string pairFirstValue;
    std::string pairSecondValue;
    std::string materialTechniqueName;
    for (YAML::const_iterator seqIt = materialTechniquesNode.begin(); seqIt != materialTechniquesNode.end(); ++seqIt) {
        const YAML::Node materialMap = *seqIt;
        BRE_ASSERT(materialMap.IsMap());

        // Get material technique name
        YAML::const_iterator mapIt = materialMap.begin();
        BRE_ASSERT(mapIt != materialMap.end());
        pairFirstValue = mapIt->first.as<std::string>();
        BRE_ASSERT(pairFirstValue == std::string("name"));
        materialTechniqueName = mapIt->second.as<std::string>();
        BRE_ASSERT_MSG(mMaterialTechniqueByName.find(materialTechniqueName) == mMaterialTechniqueByName.end(),
                   L"Material technique name must be unique");
        ++mapIt;

        // Get material techniques settings (diffuse texture, normal texture, etc)
        MaterialTechnique materialTechnique;
        while (mapIt != materialMap.end()) {
            pairFirstValue = mapIt->first.as<std::string>();
            pairSecondValue = mapIt->second.as<std::string>();
            UpdateMaterialTechnique(pairFirstValue, pairSecondValue, materialTechnique);
            ++mapIt;
        }

        mMaterialTechniqueByName.insert(std::make_pair(materialTechniqueName, materialTechnique));
    }
}

const MaterialTechnique& MaterialTechniqueLoader::GetMaterialTechnique(const std::string& name) const noexcept
{
    std::unordered_map<std::string, MaterialTechnique>::const_iterator findIt = mMaterialTechniqueByName.find(name);
    BRE_ASSERT_MSG(findIt != mMaterialTechniqueByName.end(), L"Material technique name not found");

    return findIt->second;
}

void MaterialTechniqueLoader::UpdateMaterialTechnique(const std::string& materialTechniquePropertyName,
                                                      const std::string& materialTechniqueTextureName,
                                                      MaterialTechnique& materialTechnique) const noexcept
{
    ID3D12Resource& texture = mTextureLoader.GetTexture(materialTechniqueTextureName);
    if (materialTechniquePropertyName == "diffuse texture") {
        materialTechnique.SetDiffuseTexture(&texture);
    } else if (materialTechniquePropertyName == "normal texture") {
        materialTechnique.SetNormalTexture(&texture);
    } else if (materialTechniquePropertyName == "height texture") {
        materialTechnique.SetHeightTexture(&texture);
    } else {
        BRE_ASSERT(false);
    }
}
}


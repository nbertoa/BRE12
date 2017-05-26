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

    // 'material techniques' node can be undefined when all the drawable objects use
    // the color mapping technique (that is the default technique when a drawable object
    // does not specify a 'material technique' to use)
    if (materialTechniquesNode.IsDefined() == false) {
        return;
    }

    BRE_CHECK_MSG(materialTechniquesNode.IsSequence(), L"'material techniques' node must be a map");

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

        BRE_CHECK_MSG(pairFirstValue == std::string("name") || pairFirstValue == std::string("reference"),
                       L"Material techniques 1st parameter must be 'name', or it must be 'reference'");

        // If name is "reference", then path must be a yaml file that specifies "material techniques"
        if (pairFirstValue == "reference") {
            pairSecondValue = mapIt->second.as<std::string>();

            const YAML::Node referenceRootNode = YAML::LoadFile(pairSecondValue);
            const std::wstring errorMsg =
                L"Failed to open yaml file: " + StringUtils::AnsiToWideString(pairSecondValue);
            BRE_CHECK_MSG(referenceRootNode.IsDefined(), errorMsg.c_str());
            BRE_CHECK_MSG(referenceRootNode["material techniques"].IsDefined(),
                           L"Reference file must have 'material techniques' field");
            LoadMaterialTechniques(referenceRootNode);

            continue;
        }


        materialTechniqueName = mapIt->second.as<std::string>();
        const std::wstring errorMsg =
            L"Material technique name must be unique: " + StringUtils::AnsiToWideString(materialTechniqueName);
        BRE_CHECK_MSG(mMaterialTechniqueByName.find(materialTechniqueName) == mMaterialTechniqueByName.end(),
                       errorMsg.c_str());
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
    const std::wstring errorMsg =
        L"Material technique name not found: " + StringUtils::AnsiToWideString(name);
    BRE_CHECK_MSG(findIt != mMaterialTechniqueByName.end(), errorMsg.c_str());

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
        // To avoid warning about 'conditional expression is constant'. This is the same than false
        const std::wstring errorMsg =
            L"Unknown material technique field: " + StringUtils::AnsiToWideString(materialTechniquePropertyName);
        BRE_CHECK_MSG(&materialTechniquePropertyName == nullptr, errorMsg.c_str());
    }
}
}
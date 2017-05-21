#include "MaterialPropertiesLoader.h"

#pragma warning( push )
#pragma warning( disable : 4127)
#include <yaml-cpp/yaml.h>
#pragma warning( pop ) 

#include <MathUtils\MathUtils.h>
#include <SceneLoader\YamlUtils.h>
#include <Utils/DebugUtils.h>

namespace BRE {
void
MaterialPropertiesLoader::LoadMaterialsProperties(const YAML::Node& rootNode) noexcept
{
    BRE_ASSERT(rootNode.IsDefined());

    // Get the "material properties" node. It is a sequence of maps and its sintax is:
    // material properties:
    //   - name: materialName
    //     base color: [value, value, value]
    //     smoothness: value
    //     metal mask: value
    //   - name: materialName2
    //     base color: [value, value, value]
    //     smoothness: value
    //     metal mask: value
    const YAML::Node materialsNode = rootNode["material properties"];
    BRE_ASSERT_MSG(materialsNode.IsDefined(), L"'material properties' node must be defined");
    BRE_ASSERT_MSG(materialsNode.IsSequence(), L"'material propreties' node must be a map");

    std::string pairFirstValue;
    std::string materialName;
    for (YAML::const_iterator seqIt = materialsNode.begin(); seqIt != materialsNode.end(); ++seqIt) {
        const YAML::Node materialMap = *seqIt;
        BRE_ASSERT(materialMap.IsMap());

        // Get material name
        YAML::const_iterator mapIt = materialMap.begin();
        BRE_ASSERT_MSG(mapIt != materialMap.end(), L"Material name not found");
        pairFirstValue = mapIt->first.as<std::string>();
        BRE_ASSERT_MSG(pairFirstValue == std::string("name"), L"Material properties 1st parameter must be 'name'");
        materialName = mapIt->second.as<std::string>();
        BRE_ASSERT_MSG(mMaterialPropertiesByName.find(materialName) == mMaterialPropertiesByName.end(),
                       L"Material properties name must be unique");
        ++mapIt;
        BRE_ASSERT(mapIt != materialMap.end());

        // Get material properties
        float baseColor[3U] = { 0.0f, 0.0f, 0.0f };
        float smoothness = 0.0f;
        float metalMask = 0.0f;
        while (mapIt != materialMap.end()) {
            pairFirstValue = mapIt->first.as<std::string>();

            if (pairFirstValue == "base color") {
                YamlUtils::GetSequence(mapIt->second, baseColor, 3U);
            } else if (pairFirstValue == "smoothness") {
                smoothness = mapIt->second.as<float>();
                MathUtils::Clamp(smoothness, 0.0f, 1.0f);
            } else if (pairFirstValue == "metal mask") {
                metalMask = mapIt->second.as<float>();
                MathUtils::Clamp(metalMask, 0.0f, 1.0f);
            } else {
                // To avoid warning about 'conditional expression is constant'. This is the same than false
                BRE_ASSERT_MSG(&metalMask == nullptr, L"Unknown material properties field");
            }

            ++mapIt;
        }

        MaterialProperties materialProperties(baseColor[0],
                                              baseColor[1],
                                              baseColor[2],
                                              metalMask,
                                              smoothness);
        mMaterialPropertiesByName.insert(std::make_pair(materialName, materialProperties));
    }
}

const MaterialProperties& MaterialPropertiesLoader::GetMaterialProperties(const std::string& name) const noexcept
{
    std::unordered_map<std::string, MaterialProperties>::const_iterator findIt = mMaterialPropertiesByName.find(name);
    BRE_ASSERT_MSG(findIt != mMaterialPropertiesByName.end(), L"Material properties not found");

    return findIt->second;
}
}
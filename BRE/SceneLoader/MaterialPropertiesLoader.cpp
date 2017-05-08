#include "MaterialPropertiesLoader.h"

#pragma warning( push )
#pragma warning( disable : 4127)
#include <yaml-cpp/yaml.h>
#pragma warning( pop ) 

#include <MathUtils\MathUtils.h>
#include <SceneLoader\YamlUtils.h>
#include <Utils/DebugUtils.h>

void
MaterialPropertiesLoader::LoadMaterialsProperties(const YAML::Node& rootNode) noexcept
{
    ASSERT(rootNode.IsDefined());

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
    ASSERT_MSG(materialsNode.IsDefined(), L"'material properties' node must be defined");
    ASSERT_MSG(materialsNode.IsSequence(), L"'material propreties' node must be a map");

    std::string pairFirstValue;
    std::string materialName;
    float baseColor[3U];
    for (YAML::const_iterator seqIt = materialsNode.begin(); seqIt != materialsNode.end(); ++seqIt) {
        const YAML::Node materialMap = *seqIt;
        ASSERT(materialMap.IsMap());

        // Get material name
        YAML::const_iterator mapIt = materialMap.begin();
        ASSERT_MSG(mapIt != materialMap.end(), L"Material name not found");
        pairFirstValue = mapIt->first.as<std::string>();
        ASSERT(pairFirstValue == std::string("name"));
        materialName = mapIt->second.as<std::string>();
        ASSERT_MSG(mMaterialPropertiesByName.find(materialName) == mMaterialPropertiesByName.end(),
                   L"Material properties name must be unique");
        ++mapIt;
        ASSERT(mapIt != materialMap.end());

        // Get material base color
        pairFirstValue = mapIt->first.as<std::string>();
        ASSERT(pairFirstValue == std::string("base color"));
        YamlUtils::GetSequence(mapIt->second, baseColor, 3U);
        ++mapIt;
        ASSERT(mapIt != materialMap.end());

        // Get material smoothness
        pairFirstValue = mapIt->first.as<std::string>();
        ASSERT(pairFirstValue == std::string("smoothness"));
        float smoothness = mapIt->second.as<float>();
        MathUtils::Clamp(smoothness, 0.0f, 1.0f);
        ++mapIt;
        ASSERT(mapIt != materialMap.end());

        // Get material metal mask
        pairFirstValue = mapIt->first.as<std::string>();
        ASSERT(pairFirstValue == std::string("metal mask"));
        float metalMask = mapIt->second.as<float>();
        MathUtils::Clamp(metalMask, 0.0f, 1.0f);
        ++mapIt;
        ASSERT_MSG(mapIt == materialMap.end(), L"More fields for material property than expected");

        MaterialProperties materialProperties(baseColor[0],
                                              baseColor[1],
                                              baseColor[2],
                                              smoothness,
                                              metalMask);
        mMaterialPropertiesByName.insert(std::make_pair(materialName, materialProperties));
    }
}

const MaterialProperties& MaterialPropertiesLoader::GetMaterialProperties(const std::string& name) const noexcept
{
    std::unordered_map<std::string, MaterialProperties>::const_iterator findIt = mMaterialPropertiesByName.find(name);
    ASSERT_MSG(findIt != mMaterialPropertiesByName.end(), L"Material not found");

    return findIt->second;
}
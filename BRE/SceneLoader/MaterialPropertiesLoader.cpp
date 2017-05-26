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
    const YAML::Node materialPropertiesNode = rootNode["material properties"];

    // 'material properties' node can be undefined.
    if (materialPropertiesNode.IsDefined() == false) {
        return;
    }

    BRE_CHECK_MSG(materialPropertiesNode.IsSequence(), L"'material properties' node must be a map");

    std::string pairFirstValue;
    std::string pairSecondValue;
    for (YAML::const_iterator seqIt = materialPropertiesNode.begin(); seqIt != materialPropertiesNode.end(); ++seqIt) {
        const YAML::Node materialMap = *seqIt;
        BRE_ASSERT(materialMap.IsMap());

        // Get material name
        YAML::const_iterator mapIt = materialMap.begin();
        BRE_CHECK_MSG(mapIt != materialMap.end(), L"Material name not found");
        pairFirstValue = mapIt->first.as<std::string>();

        BRE_CHECK_MSG(pairFirstValue == std::string("name") || pairFirstValue == std::string("reference"),
                       L"Material properties 1st parameter must be 'name', or it must be 'reference'");

        pairSecondValue = mapIt->second.as<std::string>();

        // If the item is 'reference', then path must be a yaml file that specifies "material properties"
        if (pairFirstValue == std::string("reference")) {
            const YAML::Node referenceRootNode = YAML::LoadFile(pairSecondValue);
            BRE_CHECK_MSG(referenceRootNode.IsDefined(), L"Failed to open yaml file");
            BRE_CHECK_MSG(referenceRootNode["material properties"].IsDefined(),
                           L"Reference file must have 'material properties' field");
            LoadMaterialsProperties(referenceRootNode);

            continue;
        }

        // Otherwise, we get the material properties

        BRE_CHECK_MSG(mMaterialPropertiesByName.find(pairSecondValue) == mMaterialPropertiesByName.end(),
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
                const std::wstring errorMsg =
                    L"Unknown material properties field: " + StringUtils::AnsiToWideString(pairFirstValue);
                BRE_CHECK_MSG(&metalMask == nullptr, errorMsg.c_str());
            }

            ++mapIt;
        }

        MaterialProperties materialProperties(baseColor[0],
                                              baseColor[1],
                                              baseColor[2],
                                              metalMask,
                                              smoothness);
        mMaterialPropertiesByName.insert(std::make_pair(pairSecondValue, materialProperties));
    }
}

const MaterialProperties& MaterialPropertiesLoader::GetMaterialProperties(const std::string& name) const noexcept
{
    std::unordered_map<std::string, MaterialProperties>::const_iterator findIt = mMaterialPropertiesByName.find(name);
    const std::wstring errorMsg =
        L"Material properties not found: " + StringUtils::AnsiToWideString(name);
    BRE_CHECK_MSG(findIt != mMaterialPropertiesByName.end(), errorMsg.c_str());

    return findIt->second;
}
}
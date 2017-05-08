#include "DrawableObjectLoader.h"

#pragma warning( push )
#pragma warning( disable : 4127)
#include <yaml-cpp/yaml.h>
#pragma warning( pop ) 

#include <SceneLoader\MaterialPropertiesLoader.h>
#include <SceneLoader\MaterialTechniqueLoader.h>
#include <SceneLoader\ModelLoader.h>
#include <SceneLoader\YamlUtils.h>
#include <Utils/DebugUtils.h>

using namespace DirectX;

namespace BRE {
void
DrawableObjectLoader::LoadDrawableObjects(const YAML::Node& rootNode) noexcept
{
    BRE_ASSERT(rootNode.IsDefined());

    // Get the "drawable objects" node. It is a sequence of maps and its sintax is:
    // drawable objects:
    //   - model: modelName
    //     material properties: materialPropertiesName
    //     material technique: drawableObjectName
    //     translation: [10.0, 0.0, 12.0]
    //     rotation: [3.14, 0.0, 0.0]
    //     scale: [1, 1, 3]
    //   - model: modelName
    //     material properties: materialPropertiesName
    //     material technique: drawableObjectName
    //     scale: [1, 3, 3]
    const YAML::Node drawableObjectsNode = rootNode["drawable objects"];
    BRE_ASSERT_MSG(drawableObjectsNode.IsDefined(), L"'drawable objects' node must be defined");
    BRE_ASSERT_MSG(drawableObjectsNode.IsSequence(), L"'drawable objects' node must be a map");

    // We need model name to fill mDrawableObjectsByModelName
    std::string modelName;
    std::string pairFirstValue;
    std::string pairSecondValue;
    for (YAML::const_iterator seqIt = drawableObjectsNode.begin(); seqIt != drawableObjectsNode.end(); ++seqIt) {
        const YAML::Node drawableObjectMap = *seqIt;
        BRE_ASSERT_MSG(drawableObjectMap.IsMap(), L"Each drawable object must be a map");

        // Get data to build drawable object
        const Model* model = nullptr;
        const MaterialProperties* materialProperties = nullptr;
        const MaterialTechnique* materialTechnique = nullptr;
        float translation[3]{ 0.0f, 0.0f, 0.0f };
        float rotation[3]{ 0.0f, 0.0f, 0.0f };
        float scale[3]{ 1.0f, 1.0f, 1.0f };
        YAML::const_iterator mapIt = drawableObjectMap.begin();
        while (mapIt != drawableObjectMap.end()) {
            pairFirstValue = mapIt->first.as<std::string>();

            if (pairFirstValue == "model") {
                BRE_ASSERT_MSG(model == nullptr, L"Drawable object model must be set once");
                pairSecondValue = mapIt->second.as<std::string>();
                modelName = pairSecondValue;
                model = &mModelLoader.GetModel(pairSecondValue);
            } else if (pairFirstValue == "material properties") {
                BRE_ASSERT_MSG(materialProperties == nullptr, L"Drawable object material properties must be set once");
                pairSecondValue = mapIt->second.as<std::string>();
                materialProperties = &mMaterialPropertiesLoader.GetMaterialProperties(pairSecondValue);
            } else if (pairFirstValue == "material technique") {
                BRE_ASSERT_MSG(materialTechnique == nullptr, L"Drawable object material technique must be set once");
                pairSecondValue = mapIt->second.as<std::string>();
                materialTechnique = &mMaterialTechniqueLoader.GetMaterialTechnique(pairSecondValue);
            } else if (pairFirstValue == "translation") {
                YamlUtils::GetSequence(mapIt->second, translation, 3U);
            } else if (pairFirstValue == "rotation") {
                YamlUtils::GetSequence(mapIt->second, rotation, 3U);
            } else if (pairFirstValue == "scale") {
                YamlUtils::GetSequence(mapIt->second, scale, 3U);
            } else {
                BRE_ASSERT(false);
            }

            ++mapIt;
        }

        BRE_ASSERT_MSG(model != nullptr, L"'model' field was not present in current drawable object");
        BRE_ASSERT_MSG(materialProperties != nullptr, L"'material properties' field was not present in current drawable object");
        BRE_ASSERT_MSG(materialTechnique != nullptr, L"'material technique' field was not present in current drawable object");

        // Build worldMatrix
        XMFLOAT4X4 worldMatrix;
        MathUtils::ComputeMatrix(worldMatrix,
                                 translation[0],
                                 translation[1],
                                 translation[2],
                                 scale[0],
                                 scale[1],
                                 scale[2],
                                 rotation[0],
                                 rotation[1],
                                 rotation[2]);

        DrawableObject drawableObject(*model,
                                      *materialProperties,
                                      *materialTechnique,
                                      worldMatrix);

        DrawableObjectsByModelName& drawableObjectsByModelName = mDrawableObjectsByModelName[materialTechnique->GetType()];
        drawableObjectsByModelName[modelName].emplace_back(drawableObject);
    }
}
}


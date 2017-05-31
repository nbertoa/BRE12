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
    BRE_CHECK_MSG(drawableObjectsNode.IsDefined(), L"'drawable objects' node must be defined");
    BRE_CHECK_MSG(drawableObjectsNode.IsSequence(), L"'drawable objects' node must be a sequence");

    // We need model name to fill mDrawableObjectsByModelName
    std::string modelName;
    std::string pairFirstValue;
    std::string pairSecondValue;
    for (YAML::const_iterator seqIt = drawableObjectsNode.begin(); seqIt != drawableObjectsNode.end(); ++seqIt) {
        const YAML::Node drawableObjectMap = *seqIt;
        BRE_CHECK_MSG(drawableObjectMap.IsMap(), L"Each drawable object must be a map");

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
                BRE_CHECK_MSG(model == nullptr, L"Drawable object model must be set once");
                pairSecondValue = mapIt->second.as<std::string>();
                modelName = pairSecondValue;
                model = &mModelLoader.GetModel(pairSecondValue);
            } else if (pairFirstValue == "material properties") {
                BRE_CHECK_MSG(materialProperties == nullptr, L"Drawable object material properties must be set once");
                pairSecondValue = mapIt->second.as<std::string>();
                materialProperties = &mMaterialPropertiesLoader.GetMaterialProperties(pairSecondValue);
            } else if (pairFirstValue == "material technique") {
                BRE_CHECK_MSG(materialTechnique == nullptr, L"Drawable object material technique must be set once");
                pairSecondValue = mapIt->second.as<std::string>();
                materialTechnique = &mMaterialTechniqueLoader.GetMaterialTechnique(pairSecondValue);
            } else if (pairFirstValue == "translation") {
                YamlUtils::GetSequence(mapIt->second, translation, 3U);
            } else if (pairFirstValue == "rotation") {
                YamlUtils::GetSequence(mapIt->second, rotation, 3U);
            } else if (pairFirstValue == "scale") {
                YamlUtils::GetSequence(mapIt->second, scale, 3U);
            } else if (pairFirstValue == "reference") {
                // If the first field is "reference", then the second field must be a yaml file 
                // that specifies "drawable objects"
                const YAML::Node referenceRootNode = YAML::LoadFile(pairSecondValue);
                BRE_CHECK_MSG(referenceRootNode.IsDefined(), L"Failed to open yaml file");
                BRE_CHECK_MSG(referenceRootNode["drawable objects"].IsDefined(),
                              L"Reference file must have 'drawable objects' field");
                LoadDrawableObjects(referenceRootNode);
            } else {
                // To avoid warning about 'conditional expression is constant'. This is the same than false
                const std::wstring errorMsg =
                    L"Unknown drawable object field: " + StringUtils::AnsiToWideString(pairFirstValue);
                BRE_CHECK_MSG(&scale == nullptr, errorMsg.c_str());
            }

            ++mapIt;
        }

        BRE_CHECK_MSG(model != nullptr, L"'model' field was not present in current drawable object");

        // If "material technique" field is not present, then it defaults to "color mapping" technique
        if (materialTechnique == nullptr) {
            materialTechnique = &mMaterialTechniqueLoader.GetDefaultMaterialTechnique();
        }

        // If "material properties" field is not present, then we get the default material properties
        if (materialProperties == nullptr) {
            materialProperties = &mMaterialPropertiesLoader.GetDefaultMaterialProperties();
        }

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
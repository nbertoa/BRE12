#include "EnvironmentLoader.h"

#pragma warning( push )
#pragma warning( disable : 4127)
#include <yaml-cpp/yaml.h>
#pragma warning( pop ) 

#include <SceneLoader\TextureLoader.h>

namespace BRE {
void
EnvironmentLoader::LoadEnvironment(const YAML::Node& rootNode) noexcept
{
    BRE_ASSERT(rootNode.IsDefined());

    // Get the "environment" node. It is a single sequence of maps and its sintax is:
    // environment:
    //   - environment texture: textureName
    //     diffuse irradiance texture: textureName
    //     specular pre convolved environment texture: textureName
    const YAML::Node environmentNode = rootNode["environment"];
    BRE_ASSERT_MSG(environmentNode.IsDefined(), L"'environment' node must be defined");
    BRE_ASSERT_MSG(environmentNode.IsSequence(), L"'environment' node must be a sequence");

    BRE_ASSERT(environmentNode.begin() != environmentNode.end());
    const YAML::Node environmentMap = *environmentNode.begin();
    BRE_ASSERT_MSG(environmentMap.IsMap(), L"'environment' node first sequence element must be a map");

    // Fill the environment textures
    YAML::const_iterator mapIt = environmentMap.begin();
    std::string pairFirstValue;
    std::string pairSecondValue;
    while (mapIt != environmentMap.end()) {
        pairFirstValue = mapIt->first.as<std::string>();
        pairSecondValue = mapIt->second.as<std::string>();
        UpdateEnvironmentTexture(pairFirstValue, pairSecondValue);
        ++mapIt;
    }

    BRE_ASSERT(mSkyBoxTexture != nullptr);
    BRE_ASSERT(mDiffuseIrradianceTexture != nullptr);
    BRE_ASSERT(mSpecularPreConvolvedEnvironmentTexture != nullptr);
}

void EnvironmentLoader::UpdateEnvironmentTexture(const std::string& environmentName,
                                                 const std::string& environmentTextureName) noexcept
{
    ID3D12Resource& texture = mTextureLoader.GetTexture(environmentTextureName);
    if (environmentName == "sky box texture") {
        BRE_ASSERT_MSG(mSkyBoxTexture == nullptr, L"Sky box texture must be set once");
        mSkyBoxTexture = &texture;
    } else if (environmentName == "diffuse irradiance texture") {
        BRE_ASSERT_MSG(mDiffuseIrradianceTexture == nullptr, L"Diffuse irradiance texture must be set once");
        mDiffuseIrradianceTexture = &texture;
    } else if (environmentName == "specular pre convolved environment texture") {
        BRE_ASSERT_MSG(mSpecularPreConvolvedEnvironmentTexture == nullptr,
                   L"Specular pre convolved enviroment texture must be set once");
        mSpecularPreConvolvedEnvironmentTexture = &texture;
    } else {
        BRE_ASSERT(false);
    }
}
}


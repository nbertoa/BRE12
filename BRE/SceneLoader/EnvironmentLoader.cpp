#include "EnvironmentLoader.h"

#pragma warning( push )
#pragma warning( disable : 4127)
#include <yaml-cpp/yaml.h>
#pragma warning( pop ) 

#include <SceneLoader\TextureLoader.h>

void EnvironmentLoader::LoadEnvironment(const YAML::Node& rootNode) noexcept {
	ASSERT(rootNode.IsDefined());

	// Get the "environment" node. It is a single sequence of maps and its sintax is:
	// environment:
	//   - environment texture: textureName
	//     diffuse irradiance texture: textureName
	//     specular pre convolved environment texture: textureName
	const YAML::Node environmentNode = rootNode["environment"];
	ASSERT(environmentNode.IsDefined());
	ASSERT(environmentNode.IsSequence());

	ASSERT(environmentNode.begin() != environmentNode.end());
	const YAML::Node environmentMap = *environmentNode.begin();
	ASSERT(environmentMap.IsMap());
	
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

	ASSERT(mSkyBoxTexture != nullptr);
	ASSERT(mDiffuseIrradianceTexture != nullptr);
	ASSERT(mSpecularPreConvolvedEnvironmentTexture != nullptr);
}

void EnvironmentLoader::UpdateEnvironmentTexture(
	const std::string& environmentName,
	const std::string& environmentTextureName) noexcept
{
	ID3D12Resource& texture = mTextureLoader.GetTexture(environmentTextureName);
	if (environmentName == "sky box texture") {
		ASSERT(mSkyBoxTexture == nullptr);
		mSkyBoxTexture = &texture;
	}
	else if (environmentName == "diffuse irradiance texture") {
		ASSERT(mDiffuseIrradianceTexture == nullptr);
		mDiffuseIrradianceTexture = &texture;
	}
	else if (environmentName == "specular pre convolved environment texture") {
		ASSERT(mSpecularPreConvolvedEnvironmentTexture == nullptr);
		mSpecularPreConvolvedEnvironmentTexture = &texture;
	}
	else {
		ASSERT(false);
	}
}
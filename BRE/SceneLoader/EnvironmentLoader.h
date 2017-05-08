#pragma once

#include <Utils\DebugUtils.h>

namespace YAML {
	class Node;
}

struct ID3D12Resource;

class TextureLoader;

class EnvironmentLoader {
public:
	EnvironmentLoader(TextureLoader& textureLoader)
		: mTextureLoader(textureLoader)
	{}
	EnvironmentLoader(const EnvironmentLoader&) = delete;
	const EnvironmentLoader& operator=(const EnvironmentLoader&) = delete;
	EnvironmentLoader(EnvironmentLoader&&) = delete;
	EnvironmentLoader& operator=(EnvironmentLoader&&) = delete;

	void LoadEnvironment(const YAML::Node& rootNode) noexcept;

	ID3D12Resource& GetSkyBoxTexture() const noexcept { 
		ASSERT(mSkyBoxTexture != nullptr);
		return *mSkyBoxTexture;
	}

	ID3D12Resource& GetDiffuseIrradianceTexture() const noexcept {
		ASSERT(mDiffuseIrradianceTexture != nullptr);
		return *mDiffuseIrradianceTexture;
	}

	ID3D12Resource& GetSpecularPreConvolvedEnvironmentTexture() const noexcept {
		ASSERT(mSpecularPreConvolvedEnvironmentTexture != nullptr);
		return *mSpecularPreConvolvedEnvironmentTexture;
	}

private:
	void UpdateEnvironmentTexture(		const std::string& environmentName,
		const std::string& environmentTextureName) noexcept;

	TextureLoader& mTextureLoader;

	ID3D12Resource* mSkyBoxTexture{ nullptr };
	ID3D12Resource* mDiffuseIrradianceTexture{ nullptr };
	ID3D12Resource* mSpecularPreConvolvedEnvironmentTexture{ nullptr };
};
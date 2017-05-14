#pragma once

#include <Utils\DebugUtils.h>

namespace YAML {
	class Node;
}

struct ID3D12Resource;

namespace BRE {
class TextureLoader;

///
/// @brief Responsible to load from scene file the environment configurations
///
class EnvironmentLoader {
public:
    EnvironmentLoader(TextureLoader& textureLoader)
        : mTextureLoader(textureLoader)
    {}
    EnvironmentLoader(const EnvironmentLoader&) = delete;
    const EnvironmentLoader& operator=(const EnvironmentLoader&) = delete;
    EnvironmentLoader(EnvironmentLoader&&) = delete;
    EnvironmentLoader& operator=(EnvironmentLoader&&) = delete;

    ///
    /// @brief Load environment
    /// @param rootNode Scene YAML file root node
    ///
    void LoadEnvironment(const YAML::Node& rootNode) noexcept;

    ///
    /// @brief Get sky box texture
    /// @return Sky box texture
    ///
    ID3D12Resource& GetSkyBoxTexture() const noexcept
    {
        BRE_ASSERT(mSkyBoxTexture != nullptr);
        return *mSkyBoxTexture;
    }

    ///
    /// @brief Get diffuse irradiance environment texture
    /// @return Diffuse irradiance environment resource
    ///
    ID3D12Resource& GetDiffuseIrradianceTexture() const noexcept
    {
        BRE_ASSERT(mDiffuseIrradianceTexture != nullptr);
        return *mDiffuseIrradianceTexture;
    }

    ///
    /// @brief Get specular pre convolved environment texture
    /// @return Specular pre convolved environment resource
    ///
    ID3D12Resource& GetSpecularPreConvolvedEnvironmentTexture() const noexcept
    {
        BRE_ASSERT(mSpecularPreConvolvedEnvironmentTexture != nullptr);
        return *mSpecularPreConvolvedEnvironmentTexture;
    }

private:
    ///
    /// @brief Update environment texture
    /// @param environmentName Environment property name
    /// @param environmentTextureName Environment texture name
    ///
    void UpdateEnvironmentTexture(const std::string& environmentPropertyName,
                                  const std::string& environmentTextureName) noexcept;

    TextureLoader& mTextureLoader;

    ID3D12Resource* mSkyBoxTexture{ nullptr };
    ID3D12Resource* mDiffuseIrradianceTexture{ nullptr };
    ID3D12Resource* mSpecularPreConvolvedEnvironmentTexture{ nullptr };
};
}


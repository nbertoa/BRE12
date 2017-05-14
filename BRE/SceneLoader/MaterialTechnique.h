#pragma once

#include <Utils\DebugUtils.h>

struct ID3D12Resource;

namespace BRE {
///
/// @brief Contains material technique data like diffuse texture, normal texture, height texture, etc.
///
class MaterialTechnique {
public:
    enum TechniqueType {
        COLOR_MAPPING = 0,
        COLOR_NORMAL_MAPPING,
        COLOR_HEIGHT_MAPPING,
        TEXTURE_MAPPING,
        NORMAL_MAPPING,
        HEIGHT_MAPPING,
        NUM_TECHNIQUES,
    };

    MaterialTechnique(ID3D12Resource* diffuseTexture = nullptr,
                      ID3D12Resource* normalTexture = nullptr,
                      ID3D12Resource* heightTexture = nullptr)
        : mDiffuseTexture(diffuseTexture)
        , mNormalTexture(normalTexture)
        , mHeightTexture(heightTexture)
    {}

    ///
    /// @brief Get diffuse texture
    /// @return Diffuse texture
    ///
    ID3D12Resource& GetDiffuseTexture() const noexcept
    {
        BRE_ASSERT(mDiffuseTexture != nullptr);
        return *mDiffuseTexture;
    }

    ///
    /// @brief Set diffuse texture
    /// @param texture New diffuse texture.
    ///
    void SetDiffuseTexture(ID3D12Resource* texture) noexcept
    {
        BRE_ASSERT(texture != nullptr);
        mDiffuseTexture = texture;
    }

    ///
    /// @brief Get normal texture
    /// @return Normal texture
    ///
    ID3D12Resource& GetNormalTexture() const noexcept
    {
        BRE_ASSERT(mNormalTexture != nullptr);
        return *mNormalTexture;
    }

    ///
    /// @brief Set normal texture
    /// @param texture New normal texture
    ///
    void SetNormalTexture(ID3D12Resource* texture) noexcept
    {
        BRE_ASSERT(texture != nullptr);
        mNormalTexture = texture;
    }

    ///
    /// @brief Get height texture
    /// @return Height texture
    ///
    ID3D12Resource& GetHeightTexture() const noexcept
    {
        BRE_ASSERT(mHeightTexture != nullptr);
        return *mHeightTexture;
    }

    ///
    /// @brief Set height texture
    /// @param texture New height texture
    ///
    void SetHeightTexture(ID3D12Resource* texture) noexcept
    {
        BRE_ASSERT(texture != nullptr);
        mHeightTexture = texture;
    }

    ///
    /// @brief Get material technique type
    /// @return Material technique type
    ///
    TechniqueType GetType() const noexcept;

private:
    ID3D12Resource* mDiffuseTexture{ nullptr };
    ID3D12Resource* mNormalTexture{ nullptr };
    ID3D12Resource* mHeightTexture{ nullptr };
};
}


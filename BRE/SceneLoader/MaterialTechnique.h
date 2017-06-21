#pragma once

#include <Utils\DebugUtils.h>

struct ID3D12Resource;

namespace BRE {
///
/// @brief Contains material technique data like base color texture, normal texture, height texture, etc.
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

    MaterialTechnique(ID3D12Resource* baseColorTexture = nullptr,
                      ID3D12Resource* metalnessTexture = nullptr,
                      ID3D12Resource* roughnessTexture = nullptr,
                      ID3D12Resource* normalTexture = nullptr,
                      ID3D12Resource* heightTexture = nullptr)
        : mBaseColorTexture(baseColorTexture)
        , mMetalnessTexture(metalnessTexture)
        , mRoughnessTexture(roughnessTexture)
        , mNormalTexture(normalTexture)
        , mHeightTexture(heightTexture)
    {}

    ///
    /// @brief Get base color texture
    /// @return Base color texture
    ///
    ID3D12Resource& GetBaseColorTexture() const noexcept
    {
        BRE_ASSERT(mBaseColorTexture != nullptr);
        return *mBaseColorTexture;
    }

    ///
    /// @brief Get metalness texture
    /// @return Metalness texture
    ///
    ID3D12Resource& GetMetalnessTexture() const noexcept
    {
        BRE_ASSERT(mMetalnessTexture != nullptr);
        return *mMetalnessTexture;
    }

    ///
    /// @brief Get roughness texture
    /// @return Roughness texture
    ///
    ID3D12Resource& GetRoughnessTexture() const noexcept
    {
        BRE_ASSERT(mRoughnessTexture != nullptr);
        return *mRoughnessTexture;
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
    /// @brief Get height texture
    /// @return Height texture
    ///
    ID3D12Resource& GetHeightTexture() const noexcept
    {
        BRE_ASSERT(mHeightTexture != nullptr);
        return *mHeightTexture;
    }

    ///
    /// @brief Set base color texture
    /// @param texture New base color texture.
    ///
    void SetBaseColorTexture(ID3D12Resource* texture) noexcept
    {
        BRE_ASSERT(texture != nullptr);
        mBaseColorTexture = texture;
    }

    ///
    /// @brief Set metalness texture
    /// @param texture New metalness texture.
    ///
    void SetMetalnessTexture(ID3D12Resource* texture) noexcept
    {
        BRE_ASSERT(texture != nullptr);
        mMetalnessTexture = texture;
    }

    ///
    /// @brief Set roughness texture
    /// @param texture New roughness texture.
    ///
    void SetRoughnessTexture(ID3D12Resource* texture) noexcept
    {
        BRE_ASSERT(texture != nullptr);
        mRoughnessTexture = texture;
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
    ID3D12Resource* mBaseColorTexture{ nullptr };
    ID3D12Resource* mMetalnessTexture{ nullptr };
    ID3D12Resource* mRoughnessTexture{ nullptr };
    ID3D12Resource* mNormalTexture{ nullptr };
    ID3D12Resource* mHeightTexture{ nullptr };
};
}
#pragma once

#include <Utils\DebugUtils.h>

struct ID3D12Resource;

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

    ID3D12Resource& GetDiffuseTexture() const noexcept
    {
        ASSERT(mDiffuseTexture != nullptr);
        return *mDiffuseTexture;
    }

    void SetDiffuseTexture(ID3D12Resource* texture) noexcept
    {
        ASSERT(texture != nullptr);
        mDiffuseTexture = texture;
    }

    ID3D12Resource& GetNormalTexture() const noexcept
    {
        ASSERT(mNormalTexture != nullptr);
        return *mNormalTexture;
    }

    void SetNormalTexture(ID3D12Resource* texture) noexcept
    {
        ASSERT(texture != nullptr);
        mNormalTexture = texture;
    }

    ID3D12Resource& GetHeightTexture() const noexcept
    {
        ASSERT(mHeightTexture != nullptr);
        return *mHeightTexture;
    }

    void SetHeightTexture(ID3D12Resource* texture) noexcept
    {
        ASSERT(texture != nullptr);
        mHeightTexture = texture;
    }

    TechniqueType GetType() const noexcept;

private:
    ID3D12Resource* mDiffuseTexture{ nullptr };
    ID3D12Resource* mNormalTexture{ nullptr };
    ID3D12Resource* mHeightTexture{ nullptr };
};
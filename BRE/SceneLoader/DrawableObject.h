#pragma once

#include <DirectXMath.h>

#include <MathUtils\MathUtils.h>
#include <Utils\DebugUtils.h>

namespace BRE {
class MaterialTechnique;
class Model;

///
/// @brief Represents the data needed to draw an object
///
class DrawableObject {
public:
    DrawableObject(const Model& model,
                   const MaterialTechnique& materialTechnique,
                   const DirectX::XMFLOAT4X4& worldMatrix,
                   const float textureScale)
        : mModel(&model)
        , mMaterialTechnique(&materialTechnique)
        , mWorldMatrix(worldMatrix)
        , mTextureScale(textureScale)
    {}

    ///
    /// @brief Get model
    /// @return Model
    ///
    const Model& GetModel() const noexcept
    {
        BRE_ASSERT(mModel != nullptr);
        return *mModel;
    }

    ///
    /// @brief Get material technique
    /// @return Material technique
    ///
    const MaterialTechnique& GetMaterialTechnique() const noexcept
    {
        BRE_ASSERT(mMaterialTechnique != nullptr);
        return *mMaterialTechnique;
    }

    ///
    /// @brief Get world matrix
    /// @return World matrix
    ///
    const DirectX::XMFLOAT4X4& GetWorldMatrix() const noexcept
    {
        return mWorldMatrix;
    }

    ///
    /// @brief Get texture scale
    /// @return Texture scale
    ///
    float GetTextureScale() const noexcept
    {
        return mTextureScale;
    }

private:
    const Model* mModel{ nullptr };
    const MaterialTechnique* mMaterialTechnique{ nullptr };
    DirectX::XMFLOAT4X4 mWorldMatrix{ MathUtils::GetIdentity4x4Matrix() };
    float mTextureScale{ 1.0f };
};
}
#pragma once

#include <DirectXMath.h>

#include <MathUtils\MathUtils.h>
#include <Utils\DebugUtils.h>

namespace BRE {
class MaterialProperties;
class MaterialTechnique;
class Model;

///
/// @brief Represents the data needed to draw an object
///
class DrawableObject {
public:
    DrawableObject(const Model& model,
                   const MaterialProperties& materialProperties,
                   const MaterialTechnique& materialTechnique,
                   const DirectX::XMFLOAT4X4& worldMatrix)
        : mModel(&model)
        , mMaterialProperties(&materialProperties)
        , mMaterialTechnique(&materialTechnique)
        , mWorldMatrix(worldMatrix)
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
    /// @brief Get material properties
    /// @return Material properties
    ///
    const MaterialProperties& GetMaterialProperties() const noexcept
    {
        BRE_ASSERT(mMaterialProperties != nullptr);
        return *mMaterialProperties;
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

private:
    const Model* mModel{ nullptr };
    const MaterialProperties* mMaterialProperties{ nullptr };
    const MaterialTechnique* mMaterialTechnique{ nullptr };
    DirectX::XMFLOAT4X4 mWorldMatrix{ MathUtils::GetIdentity4x4Matrix() };
};

}


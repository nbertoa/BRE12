#pragma once

#include <cstring>

namespace BRE {
///
/// @brief Represents material properties like base color, metal mask and smoothness.
///
class MaterialProperties {
public:
    MaterialProperties() = default;
    MaterialProperties(const float baseColorR,
                       const float baseColorG,
                       const float baseColorB,
                       const float metalMask,
                       const float smoothness)
    {
        mBaseColor_MetalMask[0U] = baseColorR;
        mBaseColor_MetalMask[1U] = baseColorG;
        mBaseColor_MetalMask[2U] = baseColorB;
        mBaseColor_MetalMask[3U] = metalMask;
        mSmoothness = smoothness;
    }

    ~MaterialProperties() = default;
    MaterialProperties(const MaterialProperties&) = default;
    MaterialProperties(MaterialProperties&&) = default;

    const MaterialProperties& operator=(const MaterialProperties& instance)
    {
        if (this == &instance) {
            return *this;
        }

        memcpy(mBaseColor_MetalMask, instance.mBaseColor_MetalMask, sizeof(mBaseColor_MetalMask));
        mSmoothness = instance.mSmoothness;

        return *this;
    }

private:
    float mBaseColor_MetalMask[4U]{ 1.0f, 1.0f, 1.0f, 0.0f };
    float mSmoothness{ 1.0f };
    float mPadding[3U];
};
}


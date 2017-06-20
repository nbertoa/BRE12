#pragma once

#include <cstring>

namespace BRE {
///
/// @brief Represents material properties like metalness and smoothness
///
class MaterialProperties {
public:
    MaterialProperties() = default;
    MaterialProperties(const float metalness,
                       const float smoothness)
    {
        mMetalnessSmoothness[0U] = metalness;
        mMetalnessSmoothness[1U] = smoothness;
    }

    ~MaterialProperties() = default;
    MaterialProperties(const MaterialProperties&) = default;
    MaterialProperties(MaterialProperties&&) = default;

    const MaterialProperties& operator=(const MaterialProperties& instance)
    {
        if (this == &instance) {
            return *this;
        }

        memcpy(mMetalnessSmoothness, instance.mMetalnessSmoothness, sizeof(mMetalnessSmoothness));

        return *this;
    }

private:
    // We use 4 floats here: metalness + smoothness + padding
    float mMetalnessSmoothness[4U]{ 1.0f, 1.0f, 1.0f, 0.0f };
};
}
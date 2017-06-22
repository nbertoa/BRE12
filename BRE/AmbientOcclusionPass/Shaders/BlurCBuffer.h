#pragma once

#include <AmbientOcclusionPass\AmbientOcclusionSettings.h>

namespace BRE {
///
/// @brief Blur constant buffer
///
struct BlurCBuffer {
    BlurCBuffer() = default;

    ///
    /// @brief BlurCBuffer constructor
    /// @param noiseTextureDimension Noise vectors texture dimensions (e.g. 4 (4x4), 8 (8x8))
    /// These vectors where used in ambient occlusion algorithm
    ///
    BlurCBuffer(const std::uint32_t noiseTextureDimension)
        : mNoiseTextureDimension(noiseTextureDimension)
    {

    }

    ~BlurCBuffer() = default;
    BlurCBuffer(const BlurCBuffer&) = default;
    BlurCBuffer(BlurCBuffer&&) = default;
    BlurCBuffer& operator=(BlurCBuffer&&) = default;

    std::uint32_t mNoiseTextureDimension{ AmbientOcclusionSettings::sNoiseTextureDimension };
};

}
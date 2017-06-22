#pragma once

#include <cstdint>

namespace BRE {
///
/// @brief Responsible to handle ambient occlusion settings
///
class AmbientOcclusionSettings {
public:
    AmbientOcclusionSettings() = delete;
    ~AmbientOcclusionSettings() = delete;
    AmbientOcclusionSettings(const AmbientOcclusionSettings&) = delete;
    const AmbientOcclusionSettings& operator=(const AmbientOcclusionSettings&) = delete;
    AmbientOcclusionSettings(AmbientOcclusionSettings&&) = delete;
    AmbientOcclusionSettings& operator=(AmbientOcclusionSettings&&) = delete;
    
    static std::uint32_t sSampleKernelSize;
    static std::uint32_t sNoiseTextureDimension;
    static float sOcclusionRadius;
    static float sSsaoPower;
};
}
#pragma once

#include <cstdint>

namespace BRE {
///
/// @brief Responsible to handle all environment light settings
///
class EnvironmentLightSettings {
public:
    EnvironmentLightSettings() = delete;
    ~EnvironmentLightSettings() = delete;
    EnvironmentLightSettings(const EnvironmentLightSettings&) = delete;
    const EnvironmentLightSettings& operator=(const EnvironmentLightSettings&) = delete;
    EnvironmentLightSettings(EnvironmentLightSettings&&) = delete;
    EnvironmentLightSettings& operator=(EnvironmentLightSettings&&) = delete;
    
    // Ambient occlusion constants
    static std::uint32_t sSampleKernelSize;
    static std::uint32_t sNoiseTextureDimension;
    static float sOcclusionRadius;
    static float sSsaoPower;
};
}
#include "EnvironmentLightSettings.h"

namespace BRE {
// Ambient occlusion constants
std::uint32_t EnvironmentLightSettings::sSampleKernelSize{ 32U };
std::uint32_t EnvironmentLightSettings::sNoiseTextureDimension{ 4U };
float EnvironmentLightSettings::sOcclusionRadius{ 10.0f };
float EnvironmentLightSettings::sSsaoPower{ 2.0f };
}
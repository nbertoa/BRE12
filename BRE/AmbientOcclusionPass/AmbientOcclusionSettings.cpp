#include "AmbientOcclusionSettings.h"

namespace BRE {
// Ambient occlusion constants
std::uint32_t AmbientOcclusionSettings::sSampleKernelSize{ 32U };
std::uint32_t AmbientOcclusionSettings::sNoiseTextureDimension{ 4U };
float AmbientOcclusionSettings::sOcclusionRadius{ 10.0f };
float AmbientOcclusionSettings::sSsaoPower{ 2.0f };
}
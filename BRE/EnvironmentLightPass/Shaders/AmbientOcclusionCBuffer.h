#pragma once

#include <cstdint>

#include <ApplicationSettings\ApplicationSettings.h>
#include <EnvironmentLightPass\EnvironmentLightSettings.h>
#include <Utils\DebugUtils.h>

namespace BRE {
///
/// @brief Ambient occlusion constant buffer
///
struct AmbientOcclusionCBuffer {
    AmbientOcclusionCBuffer() = default;

    ///
    /// @brief AmbientOcclussionCBuffer constructor
    /// @param screenWidth Screen width
    /// @param screenHeight Screen height
    /// @param sampleKernelSize Number of samples in the kernel
    /// @param noiseTextureDimension Noise vectors texture dimensions (e.g. 4 (4x4), 8 (8x8))
    /// @param occlusionRadius Radius around the fragment of the geometry to take into account
    /// for ambient occlusion
    /// @param ssaoPower Power to sharpen the constrat in ambient occlusion
    ///
    AmbientOcclusionCBuffer(const float screenWidth,
                            const float screenHeight,
                            const std::uint32_t sampleKernelSize,
                            const std::uint32_t noiseTextureDimension,
                            const float occlusionRadius,
                            const float ssaoPower)
        : mScreenWidth(screenWidth)
        , mScreenHeight(screenHeight)
        , mSampleKernelSize(sampleKernelSize)
        , mNoiseTextureDimension(noiseTextureDimension)
        , mOcclusionRadius(occlusionRadius)
        , mSsaoPower(ssaoPower)
    {
        BRE_ASSERT(screenWidth > 0.0f);
        BRE_ASSERT(screenHeight > 0.0f);
        BRE_ASSERT(sampleKernelSize > 0U);
        BRE_ASSERT(noiseTextureDimension > 0U);
        BRE_ASSERT(occlusionRadius > 0.0f);
        BRE_ASSERT(ssaoPower > 0.0f);
    }

    ~AmbientOcclusionCBuffer() = default;
    AmbientOcclusionCBuffer(const AmbientOcclusionCBuffer&) = default;
    AmbientOcclusionCBuffer(AmbientOcclusionCBuffer&&) = default;
    AmbientOcclusionCBuffer& operator=(AmbientOcclusionCBuffer&&) = default;

    float mScreenWidth{ static_cast<float>(ApplicationSettings::sWindowWidth) };
    float mScreenHeight{ static_cast<float>(ApplicationSettings::sWindowHeight) };
    std::uint32_t mSampleKernelSize{ EnvironmentLightSettings::sSampleKernelSize };
    std::uint32_t mNoiseTextureDimension{ EnvironmentLightSettings::sNoiseTextureDimension };
    float mOcclusionRadius{ EnvironmentLightSettings::sOcclusionRadius };
    float mSsaoPower{ EnvironmentLightSettings::sSsaoPower };
};

}
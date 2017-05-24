#pragma once

#include <cstdint>

#include <ApplicationSettings\ApplicationSettings.h>

namespace BRE {
///
/// @brief Ambient occlusion constant buffer
///
struct AmbientOcclusionCBuffer {
    AmbientOcclusionCBuffer() = default;

    AmbientOcclusionCBuffer(const float screenWidth,
                            const float screenHeight,
                            const std::uint32_t sampleKernelSize,
                            const float noiseTextureDimension,
                            const float occlusionRadius,
                            const float ssaoPower)
        : mScreenWidth(screenWidth)
        , mScreenHeight(screenHeight)
        , mSampleKernelSize(sampleKernelSize)
        , mNoiseTextureDimension(noiseTextureDimension)
        , mOcclusionRadius(occlusionRadius)
        , mSsaoPower(ssaoPower)
    {

    }

    ~AmbientOcclusionCBuffer() = default;
    AmbientOcclusionCBuffer(const AmbientOcclusionCBuffer&) = default;
    AmbientOcclusionCBuffer(AmbientOcclusionCBuffer&&) = default;
    AmbientOcclusionCBuffer& operator=(AmbientOcclusionCBuffer&&) = default;

    float mScreenWidth{ static_cast<float>(ApplicationSettings::sWindowWidth) };
    float mScreenHeight{ static_cast<float>(ApplicationSettings::sWindowHeight) };
    std::uint32_t mSampleKernelSize{ ApplicationSettings::sSampleKernelSize };
    float mNoiseTextureDimension{ ApplicationSettings::sNoiseTextureDimension };
    float mOcclusionRadius{ ApplicationSettings::sOcclusionRadius };
    float mSsaoPower{ ApplicationSettings::sSsaoPower };
};

}
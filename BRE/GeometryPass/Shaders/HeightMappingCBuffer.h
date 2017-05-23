#pragma once

#include <DirectXMath.h>

#include <ApplicationSettings\ApplicationSettings.h>

namespace BRE {
///
/// @brief Height mapping constant buffer
///
struct HeightMappingCBuffer {
    HeightMappingCBuffer() = default;

    HeightMappingCBuffer(const float minTessellationDistance,
                         const float maxTessellationDistance,
                         const float minTessellationFactor,
                         const float maxTessellationFactor,
                         const float heightScale)
        : mMinTessellationDistance(minTessellationDistance)
        , mMaxTessellationDistance(maxTessellationDistance)
        , mMinTessellationFactor(minTessellationFactor)
        , mMaxTessellationFactor(maxTessellationFactor)
        , mHeightScale(heightScale)
    {

    }

    ~HeightMappingCBuffer() = default;
    HeightMappingCBuffer(const HeightMappingCBuffer&) = default;
    HeightMappingCBuffer(HeightMappingCBuffer&&) = default;
    HeightMappingCBuffer& operator=(HeightMappingCBuffer&&) = default;

    float mMinTessellationDistance{ ApplicationSettings::sMinTessellationDistance };
    float mMaxTessellationDistance{ ApplicationSettings::sMaxTessellationDistance };
    float mMinTessellationFactor{ ApplicationSettings::sMinTessellationFactor };
    float mMaxTessellationFactor{ ApplicationSettings::sMaxTessellationFactor };
    float mHeightScale{ ApplicationSettings::sHeightScale };
};

}
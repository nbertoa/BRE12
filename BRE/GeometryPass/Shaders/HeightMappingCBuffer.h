#pragma once

#include <GeometryPass\GeometrySettings.h>

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

    float mMinTessellationDistance{ GeometrySettings::sMinTessellationDistance };
    float mMaxTessellationDistance{ GeometrySettings::sMaxTessellationDistance };
    float mMinTessellationFactor{ GeometrySettings::sMinTessellationFactor };
    float mMaxTessellationFactor{ GeometrySettings::sMaxTessellationFactor };
    float mHeightScale{ GeometrySettings::sHeightScale };
};

}
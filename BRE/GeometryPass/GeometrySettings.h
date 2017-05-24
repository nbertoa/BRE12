#pragma once

#include <cstdint>

namespace BRE {
///
/// @brief Responsible to handle all geometry pass settings
///
class GeometrySettings {
public:
    GeometrySettings() = delete;
    ~GeometrySettings() = delete;
    GeometrySettings(const GeometrySettings&) = delete;
    const GeometrySettings& operator=(const GeometrySettings&) = delete;
    GeometrySettings(GeometrySettings&&) = delete;
    GeometrySettings& operator=(GeometrySettings&&) = delete;
    
    // Height mapping constants
    static float sMinTessellationDistance;
    static float sMaxTessellationDistance;
    static float sMinTessellationFactor;
    static float sMaxTessellationFactor;
    static float sHeightScale;
};
}
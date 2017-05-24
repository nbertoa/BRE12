#pragma once

#include <ApplicationSettings\ApplicationSettings.h>

namespace BRE {
///
/// @brief Blur constant buffer
///
struct BlurCBuffer {
    BlurCBuffer() = default;

    BlurCBuffer(const float noiseTextureDimension)
        : mNoiseTextureDimension(noiseTextureDimension)
    {

    }

    ~BlurCBuffer() = default;
    BlurCBuffer(const BlurCBuffer&) = default;
    BlurCBuffer(BlurCBuffer&&) = default;
    BlurCBuffer& operator=(BlurCBuffer&&) = default;

    float mNoiseTextureDimension{ ApplicationSettings::sNoiseTextureDimension };
};

}
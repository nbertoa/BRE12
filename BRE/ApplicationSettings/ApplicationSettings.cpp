#include "ApplicationSettings.h"

#include <cfloat>

#include <MathUtils/MathUtils.h>

namespace BRE {
const bool ApplicationSettings::sIsFullscreenWindow{ true };
const std::uint32_t ApplicationSettings::sCpuProcessorCount{ 4U }; // This should be changed according your processor
const std::uint32_t ApplicationSettings::sWindowWidth{ 1920U };
const std::uint32_t ApplicationSettings::sWindowHeight{ 1080U };

const DXGI_FORMAT ApplicationSettings::sFrameBufferRTFormat{ DXGI_FORMAT_R10G10B10A2_UNORM };
const DXGI_FORMAT ApplicationSettings::sFrameBufferFormat{ DXGI_FORMAT_R10G10B10A2_UNORM };

const DXGI_FORMAT ApplicationSettings::sColorBufferFormat{ DXGI_FORMAT_R16G16B16A16_FLOAT };
const DXGI_FORMAT ApplicationSettings::sDepthStencilFormat{ DXGI_FORMAT_R32_TYPELESS };
const DXGI_FORMAT ApplicationSettings::sDepthStencilViewFormat{ DXGI_FORMAT_D32_FLOAT };
const DXGI_FORMAT ApplicationSettings::sDepthStencilSRVFormat{ DXGI_FORMAT_R32_FLOAT };

const float ApplicationSettings::sNearPlaneZ{ 1.0f };
const float ApplicationSettings::sFarPlaneZ{ FLT_MAX };
const float ApplicationSettings::sVerticalFieldOfView{ 0.25f * MathUtils::Pi };

const D3D12_VIEWPORT ApplicationSettings::sScreenViewport{ 0.0f, 0.0f, ApplicationSettings::sWindowWidth, ApplicationSettings::sWindowHeight, 0.0f, 1.0f };
const D3D12_RECT ApplicationSettings::sScissorRect{ 0, 0, ApplicationSettings::sWindowWidth, ApplicationSettings::sWindowHeight };

const float ApplicationSettings::sSecondsPerFrame{ 1.0f / 60.0f };

// Height mapping constants
float ApplicationSettings::sMinTessellationDistance{ 25.0f };
float ApplicationSettings::sMaxTessellationDistance{ 1.0f };
float ApplicationSettings::sMinTessellationFactor{ 1.0f };
float ApplicationSettings::sMaxTessellationFactor{ 25.0f };
float ApplicationSettings::sHeightScale{ 1.5f };
}
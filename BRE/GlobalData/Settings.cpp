#include "Settings.h"

#include <cfloat>

#include <MathUtils/MathUtils.h>

const char* Settings::sResourcesPath{ "../../../external/resources/" };
const bool Settings::sFullscreen{ true };
const std::uint32_t Settings::sCpuProcessors{ 4U }; // This should be changed according your processor
const std::uint32_t Settings::sWindowWidth{ 1920U };
const std::uint32_t Settings::sWindowHeight{ 1080U };

const DXGI_FORMAT Settings::sFrameBufferRTFormat{ DXGI_FORMAT_R10G10B10A2_UNORM };
const DXGI_FORMAT Settings::sFrameBufferFormat{ DXGI_FORMAT_R10G10B10A2_UNORM };

const DXGI_FORMAT Settings::sColorBufferFormat{ DXGI_FORMAT_R16G16B16A16_FLOAT };
const DXGI_FORMAT Settings::sDepthStencilFormat{ DXGI_FORMAT_R32_TYPELESS };
const DXGI_FORMAT Settings::sDepthStencilViewFormat{ DXGI_FORMAT_D32_FLOAT };
const DXGI_FORMAT Settings::sDepthStencilSRVFormat{ DXGI_FORMAT_R32_FLOAT };

const float Settings::sNearPlaneZ{ 0.5f };
const float Settings::sFarPlaneZ{ FLT_MAX };
const float Settings::sFieldOfView{ 0.4f * MathUtils::Pi };

const D3D12_VIEWPORT Settings::sScreenViewport{ 0.0f, 0.0f, Settings::sWindowWidth, Settings::sWindowHeight, 0.0f, 1.0f };
const D3D12_RECT Settings::sScissorRect{ 0, 0, Settings::sWindowWidth, Settings::sWindowHeight };
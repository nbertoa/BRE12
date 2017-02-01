#include "SettingsManager.h"

#include <cfloat>

#include <MathUtils/MathUtils.h>

const char* SettingsManager::sResourcesPath{ "../../../external/resources/" };
const bool SettingsManager::sIsFullscreenWindow{ true };
const std::uint32_t SettingsManager::sCpuProcessorCount{ 4U }; // This should be changed according your processor
const std::uint32_t SettingsManager::sWindowWidth{ 1920U };
const std::uint32_t SettingsManager::sWindowHeight{ 1080U };

const DXGI_FORMAT SettingsManager::sFrameBufferRTFormat{ DXGI_FORMAT_R10G10B10A2_UNORM };
const DXGI_FORMAT SettingsManager::sFrameBufferFormat{ DXGI_FORMAT_R10G10B10A2_UNORM };

const DXGI_FORMAT SettingsManager::sColorBufferFormat{ DXGI_FORMAT_R16G16B16A16_FLOAT };
const DXGI_FORMAT SettingsManager::sDepthStencilFormat{ DXGI_FORMAT_R32_TYPELESS };
const DXGI_FORMAT SettingsManager::sDepthStencilViewFormat{ DXGI_FORMAT_D32_FLOAT };
const DXGI_FORMAT SettingsManager::sDepthStencilSRVFormat{ DXGI_FORMAT_R32_FLOAT };

const float SettingsManager::sNearPlaneZ{ 0.5f };
const float SettingsManager::sFarPlaneZ{ FLT_MAX };
const float SettingsManager::sVerticalFieldOfView{ 0.4f * MathUtils::Pi };

const D3D12_VIEWPORT SettingsManager::sScreenViewport{ 0.0f, 0.0f, SettingsManager::sWindowWidth, SettingsManager::sWindowHeight, 0.0f, 1.0f };
const D3D12_RECT SettingsManager::sScissorRect{ 0, 0, SettingsManager::sWindowWidth, SettingsManager::sWindowHeight };

const float SettingsManager::sSecondsPerFrame{ 1.0f / 60.0f };
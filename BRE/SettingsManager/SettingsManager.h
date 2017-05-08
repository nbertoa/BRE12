#pragma once

#include <cstdint>
#include <d3d12.h>

class SettingsManager {
public:
    SettingsManager() = delete;
    ~SettingsManager() = delete;
    SettingsManager(const SettingsManager&) = delete;
    const SettingsManager& operator=(const SettingsManager&) = delete;
    SettingsManager(SettingsManager&&) = delete;
    SettingsManager& operator=(SettingsManager&&) = delete;

    __forceinline static float AspectRatio() noexcept
    {
        return static_cast<float>(sWindowWidth) / sWindowHeight;
    }

    static const bool sIsFullscreenWindow;
    static const std::uint32_t sCpuProcessorCount;
    static const std::uint32_t sSwapChainBufferCount{ 4U };
    static const std::uint32_t sQueuedFrameCount{ sSwapChainBufferCount - 1U };
    static const std::uint32_t sWindowWidth;
    static const std::uint32_t sWindowHeight;

    // Final buffer render target
    static const DXGI_FORMAT sFrameBufferRTFormat;
    static const DXGI_FORMAT sFrameBufferFormat;

    // Color buffer used for intermediate computations (light pass, post processing passes, etc)
    static const DXGI_FORMAT sColorBufferFormat;

    // Used when creating depth stencil buffer
    static const DXGI_FORMAT sDepthStencilFormat;

    // Used when creating a view to depth stencil buffer
    static const DXGI_FORMAT sDepthStencilViewFormat;

    // Used when creating a shader resource view to the depth stencil buffer
    static const DXGI_FORMAT sDepthStencilSRVFormat;

    static const float sNearPlaneZ;
    static const float sFarPlaneZ;
    static const float sVerticalFieldOfView;

    static const D3D12_VIEWPORT sScreenViewport;
    static const D3D12_RECT sScissorRect;

    // Used to update physics. If you
    // want a fixed update time step, for example,
    // 60 FPS, then you should store 1.0f / 60.0f here
    static const float sSecondsPerFrame;
};

#pragma once

#include <cstdint>
#include <d3d12.h>

namespace BRE {
///
/// @brief Responsible to handle all main settings
///
class ApplicationSettings {
public:
    ApplicationSettings() = delete;
    ~ApplicationSettings() = delete;
    ApplicationSettings(const ApplicationSettings&) = delete;
    const ApplicationSettings& operator=(const ApplicationSettings&) = delete;
    ApplicationSettings(ApplicationSettings&&) = delete;
    ApplicationSettings& operator=(ApplicationSettings&&) = delete;

    ///
    /// @brief Get aspect ratio
    /// @return Aspect ratio
    ///
    __forceinline static float GetAspectRatio() noexcept
    {
        return static_cast<float>(sWindowWidth) / sWindowHeight;
    }

    static bool sIsFullscreenWindow;
    static std::uint32_t sCpuProcessorCount;
    static const std::uint32_t sSwapChainBufferCount{ 4U };
    static const std::uint32_t sQueuedFrameCount{ sSwapChainBufferCount - 1U };
    static std::uint32_t sWindowWidth;
    static std::uint32_t sWindowHeight;

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

    static float sNearPlaneZ;
    static float sFarPlaneZ;
    static float sVerticalFieldOfView;

    static D3D12_VIEWPORT sScreenViewport;
    static D3D12_RECT sScissorRect;

    // Used to update physics. If you
    // want a fixed update time step, for example,
    // 60 FPS, then you should store 1.0f / 60.0f here
    static const float sSecondsPerFrame;
};
}
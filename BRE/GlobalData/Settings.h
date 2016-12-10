#pragma once

#include <cstdint>
#include <d3d12.h>

class Settings {
public:
	Settings() = delete;
	~Settings() = delete;
	Settings(const Settings&) = delete;
	const Settings& operator=(const Settings&) = delete;
	Settings(Settings&&) = delete;
	Settings& operator=(Settings&&) = delete;

	__forceinline static float AspectRatio() noexcept { return static_cast<float>(sWindowWidth) / sWindowHeight; }

	static const char* sResourcesPath;
	static const bool sFullscreen;
	static const std::uint32_t sCpuProcessors;
	static const std::uint32_t sSwapChainBufferCount{ 4U };
	static const std::uint32_t sQueuedFrameCount{ sSwapChainBufferCount - 1U };
	static const std::uint32_t sWindowWidth;
	static const std::uint32_t sWindowHeight;

	// Final buffer render target
	static const DXGI_FORMAT sFrameBufferRTFormat;
	static const DXGI_FORMAT sFrameBufferFormat;

	// Color buffer used for intermediate computations (light pass, post processing passes, etc)
	static const DXGI_FORMAT sColorBufferFormat;

	// Depth stencil buffer format, used when creating depth stencil buffer
	static const DXGI_FORMAT sDepthStencilFormat;

	// Depth stencil view format, used when creating a view to depth stencil buffer
	static const DXGI_FORMAT sDepthStencilViewFormat;

	// Depth stencil shader resource view format, used when creating a srv to the depth stencil buffer
	static const DXGI_FORMAT sDepthStencilSRVFormat;

	static const float sNearPlaneZ;
	static const float sFarPlaneZ;
	static const float sFieldOfView;
	
	static const D3D12_VIEWPORT sScreenViewport;
	static const D3D12_RECT sScissorRect;
};

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
	static const bool sFullscreen{ true };
	static const std::uint32_t sCpuProcessors{ 4U }; // This should be changed according your processor
	static const std::uint32_t sSwapChainBufferCount{ 4U };
	static const std::uint32_t sQueuedFrameCount{ sSwapChainBufferCount - 1U };
	static const std::uint32_t sWindowWidth{ 1920U };
	static const std::uint32_t sWindowHeight{ 1080U };

	// Final buffer render target
	static const DXGI_FORMAT sFrameBufferRTFormat{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB };

	// Color buffer used for intermediate computations (light pass, post processing passes, etc)
	static const DXGI_FORMAT sColorBufferFormat{ DXGI_FORMAT_R16G16B16A16_FLOAT };

	// Depth stencil buffer format, used when creating depth stencil buffer
	static const DXGI_FORMAT sDepthStencilFormat{ DXGI_FORMAT_R32_TYPELESS };

	// Depth stencil view format, used when creating a view to depth stencil buffer
	static const DXGI_FORMAT sDepthStencilViewFormat{ DXGI_FORMAT_D32_FLOAT };

	// Depth stencil shader resource view format, used when creating a srv to the depth stencil buffer
	static const DXGI_FORMAT sDepthStencilSRVFormat{ DXGI_FORMAT_R32_FLOAT };

	static const float sNearPlaneZ;
	static const float sFarPlaneZ;
	static const float sFieldOfView;
	
	static const D3D12_VIEWPORT sScreenViewport;
	static const D3D12_RECT sScissorRect;
};

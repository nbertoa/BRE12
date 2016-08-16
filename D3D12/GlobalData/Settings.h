#pragma once

#include <cstdint>
#include <d3d12.h>

class Settings {
public:
	Settings() = delete;
	Settings(const Settings&) = delete;
	const Settings& operator=(const Settings&) = delete;

	__forceinline static float AspectRatio() noexcept { return (float)sWindowWidth / sWindowHeight; }

	static const bool sFullscreen{ true };
	static const std::uint32_t sCpuProcessors{ 4U }; // This should be changed according your processor
	static const std::uint32_t sSwapChainBufferCount{ 2U };
	static const std::uint32_t sQueuedFrameCount{ sSwapChainBufferCount - 1U };
	static const std::uint32_t sWindowWidth{ 1920U };
	static const std::uint32_t sWindowHeight{ 1080U };
	static const float sNearPlaneZ;
	static const float sFarPlaneZ;
	static const float sFieldOfView;
	
	static const D3D12_VIEWPORT sScreenViewport;
	static const D3D12_RECT sScissorRect;
};

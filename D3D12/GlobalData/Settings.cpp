#include "Settings.h"

const DXGI_FORMAT Settings::sRTVFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB };
const D3D12_VIEWPORT Settings::sScreenViewport{ 0.0f, 0.0f, Settings::sWindowWidth, Settings::sWindowHeight, 0.0f, 1.0f };
const D3D12_RECT Settings::sScissorRect{ 0, 0, Settings::sWindowWidth, Settings::sWindowHeight };
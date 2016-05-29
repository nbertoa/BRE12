#pragma once

#include <d3d12.h>
#include <vector>

class D3DFactory {
public:
	static D3D12_RASTERIZER_DESC DefaultRasterizerDesc() noexcept;
	static D3D12_DEPTH_STENCIL_DESC DefaultDepthStencilDesc() noexcept;
	static D3D12_BLEND_DESC DefaultBlendDesc() noexcept;

	static std::vector<D3D12_INPUT_ELEMENT_DESC> PosInputLayout() noexcept;
};

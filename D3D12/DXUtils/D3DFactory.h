#pragma once

#include <d3d12.h>
#include <vector>

// Used to generate common used data
class D3DFactory {
public:
	static D3D12_RASTERIZER_DESC DefaultRasterizerDesc() noexcept;
	static D3D12_DEPTH_STENCIL_DESC DefaultDepthStencilDesc() noexcept;
	static D3D12_BLEND_DESC DefaultBlendDesc() noexcept;
	static D3D12_HEAP_PROPERTIES HeapProperties(const D3D12_HEAP_TYPE type) noexcept;

	static std::vector<D3D12_INPUT_ELEMENT_DESC> PosInputLayout() noexcept;
	static std::vector<D3D12_INPUT_ELEMENT_DESC> PosNormalTangentTexCoordInputLayout() noexcept;
};

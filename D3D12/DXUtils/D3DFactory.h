#pragma once

#include <d3d12.h>
#include <vector>

// Used to generate common used data
namespace D3DFactory {
	D3D12_RASTERIZER_DESC DefaultRasterizerDesc() noexcept;
	D3D12_DEPTH_STENCIL_DESC DefaultDepthStencilDesc() noexcept;
	D3D12_BLEND_DESC DefaultBlendDesc() noexcept;
	D3D12_HEAP_PROPERTIES HeapProperties(const D3D12_HEAP_TYPE type) noexcept;

	std::vector<D3D12_INPUT_ELEMENT_DESC> PosInputLayout() noexcept;
	std::vector<D3D12_INPUT_ELEMENT_DESC> PosNormalTangentTexCoordInputLayout() noexcept;
}

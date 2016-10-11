#pragma once

#include <d3d12.h>
#include <vector>

// Used to generate common used D3D data 
namespace D3DFactory {
	// FillMode = D3D12_FILL_MODE_SOLID;
	// CullMode = D3D12_CULL_MODE_BACK;
	// FrontCounterClockwise = false;
	D3D12_RASTERIZER_DESC DefaultRasterizerDesc() noexcept;

	// DepthEnable = true;
	// DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	// DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	// StencilEnable = false;
	D3D12_DEPTH_STENCIL_DESC DefaultDepthStencilDesc() noexcept;

	D3D12_DEPTH_STENCIL_DESC DisableDepthStencilDesc() noexcept;

	// Blend is disabled
	D3D12_BLEND_DESC DisabledBlendDesc() noexcept;

	D3D12_BLEND_DESC AlwaysBlendDesc() noexcept;
	
	// Different input layouts
	std::vector<D3D12_INPUT_ELEMENT_DESC> PosInputLayout() noexcept;
	std::vector<D3D12_INPUT_ELEMENT_DESC> PosNormalTangentTexCoordInputLayout() noexcept;
	std::vector<D3D12_INPUT_ELEMENT_DESC> PosTexCoordInputLayout() noexcept;
}

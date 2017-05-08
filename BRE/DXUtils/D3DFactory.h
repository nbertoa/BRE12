#pragma once

#include <d3d12.h>
#include <vector>

// To generate common used D3D data 
namespace D3DFactory {
// FillMode = D3D12_FILL_MODE_SOLID;
// CullMode = D3D12_CULL_MODE_BACK;
// FrontCounterClockwise = false;
D3D12_RASTERIZER_DESC GetDefaultRasterizerDesc() noexcept;

// DepthEnable = true;
// DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
// DepthFunc = D3D12_COMPARISON_FUNC_LESS;
// StencilEnable = false;
D3D12_DEPTH_STENCIL_DESC GetDefaultDepthStencilDesc() noexcept;

// DepthEnable = true;
// DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
// DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
// StencilEnable = false;
D3D12_DEPTH_STENCIL_DESC GetReversedZDepthStencilDesc() noexcept;

D3D12_DEPTH_STENCIL_DESC GetDisabledDepthStencilDesc() noexcept;

D3D12_BLEND_DESC GetDisabledBlendDesc() noexcept;

D3D12_BLEND_DESC GetAlwaysBlendDesc() noexcept;

std::vector<D3D12_INPUT_ELEMENT_DESC> GetPosInputLayout() noexcept;
std::vector<D3D12_INPUT_ELEMENT_DESC> GetPosNormalTangentTexCoordInputLayout() noexcept;
std::vector<D3D12_INPUT_ELEMENT_DESC> GetPosTexCoordInputLayout() noexcept;
}

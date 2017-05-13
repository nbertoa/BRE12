#pragma once

#include <d3d12.h>
#include <vector>

namespace BRE {
namespace D3DFactory {
///
/// @brief Get default rasterizer descriptor
///
/// Generate a rasterizer descriptor with the following features:
/// FillMode = D3D12_FILL_MODE_SOLID
/// CullMode = D3D12_CULL_MODE_BACK
/// FrontCounterClockwise = false
///
/// @return The rasterizer descriptor
///
D3D12_RASTERIZER_DESC GetDefaultRasterizerDesc() noexcept;

///
/// @brief Get default depth stencil descriptor
///
/// Generate a depth stencil descriptor with the following features:
/// DepthEnable = true
/// DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL
/// DepthFunc = D3D12_COMPARISON_FUNC_LESS
/// StencilEnable = false
///
/// @return The depth stencil descriptor
///
D3D12_DEPTH_STENCIL_DESC GetDefaultDepthStencilDesc() noexcept;

///
/// @brief Get reversed z depth stencil descriptor
///
/// Generate a depth stencil descriptor with the following features:
/// DepthEnable = true;
/// DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
/// DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
/// StencilEnable = false;
///
/// @return The depth stencil descriptor
///
D3D12_DEPTH_STENCIL_DESC GetReversedZDepthStencilDesc() noexcept;

///
/// @brief Get a depth stencil descriptor that disables depth test.
/// @return The depth stencil descriptor
///
D3D12_DEPTH_STENCIL_DESC GetDisabledDepthStencilDesc() noexcept;

///
/// @brief Get a blend descriptor that disables blending.
/// @return The blend descriptor
///
D3D12_BLEND_DESC GetDisabledBlendDesc() noexcept;

///
/// @brief Get a blend descriptor that enables always the blending.
/// @return The blend descriptor
///
D3D12_BLEND_DESC GetAlwaysBlendDesc() noexcept;


///
/// @brief Get an input layout of position
/// @return A list of input element descriptor
///
std::vector<D3D12_INPUT_ELEMENT_DESC> GetPositionsInputLayout() noexcept;

///
/// @brief Get an input layout of position, normal, tangent and texture coordinates.
/// @return A list of input element descriptor
///
std::vector<D3D12_INPUT_ELEMENT_DESC> GetPositionNormalTangentTexCoordInputLayout() noexcept;

///
/// @brief Get an input layout of position and texture coordinates.
/// @return A list of input element descriptor
///
std::vector<D3D12_INPUT_ELEMENT_DESC> GetPositionTexCoordInputLayout() noexcept;
}
}


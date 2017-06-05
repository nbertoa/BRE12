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
std::vector<D3D12_INPUT_ELEMENT_DESC> GetPositionInputLayout() noexcept;

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

///
/// @brief Helper method to create D3D12_RESOURCE_DESC easily.
/// @param width Resource width
/// @param height Resource height
/// @param format Resource format
/// @param flags Resource flags
/// @param dimension Resource dimension
/// @param textureLayout Texture layout
/// @param mipLevels Number of mip levels of the resource
/// @param depthOrArraySize Depth or array size
/// @param alignment Alignment
/// @param sampleDescCount Sample descriptor count
/// @param sampleDescQuality Sample descriptor quality
/// @return Resource descriptor
///
D3D12_RESOURCE_DESC GetResourceDescriptor(const std::uint64_t width,
                                          const std::uint32_t height,
                                          const DXGI_FORMAT format,
                                          const D3D12_RESOURCE_FLAGS flags,
                                          const D3D12_RESOURCE_DIMENSION dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                                          const D3D12_TEXTURE_LAYOUT textureLayout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                                          const std::uint16_t mipLevels = 1UL,
                                          const std::uint16_t depthOrArraySize = 1UL,
                                          const std::uint64_t alignment = 0UL,
                                          const std::uint32_t sampleDescCount = 1U,
                                          const std::uint32_t sampleDescQuality = 0U) noexcept;

D3D12_HEAP_PROPERTIES GetHeapProperties(const D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT,
                                        const D3D12_CPU_PAGE_PROPERTY cpuPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                                        const D3D12_MEMORY_POOL memoryPool = D3D12_MEMORY_POOL_UNKNOWN,
                                        const std::uint32_t creationNodeMask = 0U,
                                        const std::uint32_t visibleNodeMask = 0U) noexcept;

D3D12_RESOURCE_BARRIER GetTransitionResourceBarrier(ID3D12Resource& resource,
                                                    const D3D12_RESOURCE_STATES stateBefore,
                                                    const D3D12_RESOURCE_STATES stateAfter,
                                                    const std::uint32_t subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                                                    const D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE) noexcept;
}
}


#include "D3DFactory.h"

#include <cstdint>

D3D12_RASTERIZER_DESC D3DFactory::DefaultRasterizerDesc() noexcept {
	D3D12_RASTERIZER_DESC desc{};
	desc.FillMode = D3D12_FILL_MODE_SOLID;
	desc.CullMode = D3D12_CULL_MODE_BACK;
	desc.FrontCounterClockwise = false;
	desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	desc.DepthClipEnable = true;
	desc.MultisampleEnable = false;
	desc.AntialiasedLineEnable = false;
	desc.ForcedSampleCount = 0;
	desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return desc;
}

D3D12_DEPTH_STENCIL_DESC D3DFactory::DefaultDepthStencilDesc() noexcept {
	D3D12_DEPTH_STENCIL_DESC desc{};
	desc.DepthEnable = true;
	desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	desc.StencilEnable = false;
	desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	desc.FrontFace = defaultStencilOp;
	desc.BackFace = defaultStencilOp;

	return desc;
}

D3D12_BLEND_DESC D3DFactory::DefaultBlendDesc() noexcept {
	D3D12_BLEND_DESC desc{};
	desc.AlphaToCoverageEnable = FALSE;
	desc.IndependentBlendEnable = FALSE;
	const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	{
		false, false,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
	};
	for (std::int32_t i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
		desc.RenderTarget[i] = defaultRenderTargetBlendDesc;
	}

	return desc;
}

D3D12_HEAP_PROPERTIES D3DFactory::HeapProperties(const D3D12_HEAP_TYPE type) noexcept {
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = type;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	return heapProps;
}

std::vector<D3D12_INPUT_ELEMENT_DESC> D3DFactory::PosInputLayout() noexcept {
	std::vector<D3D12_INPUT_ELEMENT_DESC> desc
	{
		{ "POSITION", 0U, DXGI_FORMAT_R32G32B32A32_FLOAT, 0U, 0U, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0U }
	};

	return desc;
}

std::vector<D3D12_INPUT_ELEMENT_DESC> D3DFactory::PosNormalTangentTexCoordInputLayout() noexcept {
	std::vector<D3D12_INPUT_ELEMENT_DESC> desc
	{
		{ "POSITION", 0U, DXGI_FORMAT_R32G32B32_FLOAT, 0U, 0U, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0U },
		{ "NORMAL", 0U, DXGI_FORMAT_R32G32B32_FLOAT, 0U, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0U },
		{ "TANGENT", 0U, DXGI_FORMAT_R32G32B32_FLOAT, 0U, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0U },
		{ "TEXCOORD", 0U, DXGI_FORMAT_R32G32_FLOAT, 0U, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0U }
	};

	return desc;
}
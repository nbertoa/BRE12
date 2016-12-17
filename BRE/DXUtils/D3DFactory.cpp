#include "D3DFactory.h"

#include <cstdint>

namespace D3DFactory {
	D3D12_RASTERIZER_DESC DefaultRasterizerDesc() noexcept {
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

	D3D12_DEPTH_STENCIL_DESC DefaultDepthStencilDesc() noexcept {
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

	D3D12_DEPTH_STENCIL_DESC ReverseZDepthStencilDesc() noexcept {
		D3D12_DEPTH_STENCIL_DESC desc{};
		desc.DepthEnable = true;
		desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
		desc.StencilEnable = false;
		desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
		desc.FrontFace = defaultStencilOp;
		desc.BackFace = defaultStencilOp;

		return desc;
	}

	D3D12_DEPTH_STENCIL_DESC DisableDepthStencilDesc() noexcept {
		D3D12_DEPTH_STENCIL_DESC desc{};
		desc.DepthEnable = false;
		desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		desc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc.StencilEnable = false;
		desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
		desc.FrontFace = defaultStencilOp;
		desc.BackFace = defaultStencilOp;

		return desc;
	}

	D3D12_BLEND_DESC DisabledBlendDesc() noexcept {
		D3D12_BLEND_DESC desc{};
		desc.AlphaToCoverageEnable = false;
		desc.IndependentBlendEnable = false;
		D3D12_RENDER_TARGET_BLEND_DESC rtBlendDesc{};
		rtBlendDesc.BlendEnable = false;
		rtBlendDesc.LogicOpEnable = false;
		rtBlendDesc.SrcBlend = D3D12_BLEND_ONE;
		rtBlendDesc.DestBlend = D3D12_BLEND_ZERO;
		rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
		rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
		rtBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
		rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		rtBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
		rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		for (std::int32_t i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
			desc.RenderTarget[i] = rtBlendDesc;
		}

		return desc;
	}

	D3D12_BLEND_DESC AlwaysBlendDesc() noexcept {
		D3D12_BLEND_DESC desc{};
		desc.AlphaToCoverageEnable = true;
		desc.IndependentBlendEnable = false;
		D3D12_RENDER_TARGET_BLEND_DESC rtBlendDesc{};
		rtBlendDesc.BlendEnable = true;
		rtBlendDesc.LogicOpEnable = false;
		rtBlendDesc.SrcBlend = D3D12_BLEND_ONE;
		rtBlendDesc.DestBlend = D3D12_BLEND_ONE;
		rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
		rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
		rtBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
		rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		rtBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
		rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		for (std::int32_t i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
			desc.RenderTarget[i] = rtBlendDesc;
		}

		return desc;
	}

	std::vector<D3D12_INPUT_ELEMENT_DESC> PosInputLayout() noexcept {
		std::vector<D3D12_INPUT_ELEMENT_DESC> desc
		{
			{ "POSITION", 0U, DXGI_FORMAT_R32G32B32A32_FLOAT, 0U, 0U, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0U }
		};

		return desc;
	}

	std::vector<D3D12_INPUT_ELEMENT_DESC> PosNormalTangentTexCoordInputLayout() noexcept {
		std::vector<D3D12_INPUT_ELEMENT_DESC> desc
		{
			{ "POSITION", 0U, DXGI_FORMAT_R32G32B32_FLOAT, 0U, 0U, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0U },
			{ "NORMAL", 0U, DXGI_FORMAT_R32G32B32_FLOAT, 0U, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0U },
			{ "TANGENT", 0U, DXGI_FORMAT_R32G32B32_FLOAT, 0U, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0U },
			{ "TEXCOORD", 0U, DXGI_FORMAT_R32G32_FLOAT, 0U, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0U }
		};

		return desc;
	}

	std::vector<D3D12_INPUT_ELEMENT_DESC> PosTexCoordInputLayout() noexcept {
		std::vector<D3D12_INPUT_ELEMENT_DESC> desc
		{
			{ "POSITION", 0U, DXGI_FORMAT_R32G32B32_FLOAT, 0U, 0U, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0U },
			{ "TEXCOORD", 0U, DXGI_FORMAT_R32G32_FLOAT, 0U, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0U }
		};

		return desc;
	}
}
#include <ShaderUtils/CBuffers.hlsli>

#include "RS.hlsl"

struct Input {
	uint mVertexId : SV_VertexID;
};

static const float2 gTexCoords[6] = {
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f)
};

ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b0);

struct Output {
	float4 mPosH : SV_POSITION;
	float3 mViewRayV : VIEW_RAY;
	float2 mTexCoordO : TEXCOORD;
};

[RootSignature(RS)]
Output main(in const Input input) {
	Output output;

	output.mTexCoordO = gTexCoords[input.mVertexId];

	// Quad covering screen in NDC space ([-1.0, 1.0] x [-1.0, 1.0] x [0.0, 1.0] x [1.0])
	output.mPosH = float4(2.0f * output.mTexCoordO.x - 1.0f, 1.0f - 2.0f * output.mTexCoordO.y, 0.0f, 1.0f);

	// Transform quad corners to view space near plane.
	const float4 ph = mul(output.mPosH, gFrameCBuffer.mInvP);
	output.mViewRayV = ph.xyz / ph.w;
	output.mViewRayV = normalize(output.mViewRayV);

	return output;
}
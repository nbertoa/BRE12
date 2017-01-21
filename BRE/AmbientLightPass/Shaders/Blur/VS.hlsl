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
	float2 mTexCoordO : TEXCOORD;
};

[RootSignature(RS)]
Output main(in const Input input) {
	Output output;

	const float2 texCoordO = gTexCoords[input.mVertexId];

	// Quad covering screen in NDC space.
	output.mPosH = float4(2.0f * texCoordO.x - 1.0f, 1.0f - 2.0f * texCoordO.y, 0.0f, 1.0f);
	output.mTexCoordO = texCoordO;

	return output;
}
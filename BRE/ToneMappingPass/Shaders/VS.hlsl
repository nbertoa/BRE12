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


struct Output {
	float4 mPosH : SV_POSITION;
};

[RootSignature(RS)]
Output main(in const Input input) {
	Output output;

	// Quad covering screen in NDC space.
	const float2 texCoord = gTexCoords[input.mVertexId];
	output.mPosH = float4(2.0f * texCoord.x - 1.0f, 1.0f - 2.0f * texCoord.y, 0.0f, 1.0f);

	return output;
}
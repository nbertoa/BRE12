#include <ShaderUtils/CBuffers.hlsli>

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
	float2 mTexCoordO : TEXCOORD0;
};

Output main(in const Input input) {
	Output output;

	// Quad covering screen in NDC space.
	output.mTexCoordO = gTexCoords[input.mVertexId];
	output.mPosH = float4(2.0f * output.mTexCoordO.x - 1.0f, 1.0f - 2.0f * output.mTexCoordO.y, 0.0f, 1.0f);

	/*output.mTexCoordO = float2((input.mVertexId << 1) & 2, input.mVertexId & 2);
	output.mPosH = float4(output.mTexCoordO * float2(2.0f, -2.0f) +
		float2(-1.0f, 1.0f), 0.0f, 1.0f);*/

	return output;
}
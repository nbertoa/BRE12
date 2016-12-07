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

ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b0);

struct Output {
	float4 mPosH : SV_POSITION;
	float3 mViewRayV : VIEW_RAY;
	float2 mTexCoordO : TEXCOORD;
};

Output main(in const Input input) {
	Output output;

	const float2 texCoordO = gTexCoords[input.mVertexId];

	// Quad covering screen in NDC space.
	output.mPosH = float4(2.0f * texCoordO.x - 1.0f, 1.0f - 2.0f * texCoordO.y, 0.0f, 1.0f);

	// Transform quad corners to view space near plane.
	const float4 ph = mul(output.mPosH, gFrameCBuffer.mInvP);
	output.mViewRayV = ph.xyz / ph.w;
	output.mTexCoordO = texCoordO;

	return output;
}
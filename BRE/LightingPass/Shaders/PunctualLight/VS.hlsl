#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Lights.hlsli>

#include "RS.hlsl"

struct Input {
	uint mVertexId : SV_VertexID;
};

ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b0);

StructuredBuffer<PunctualLight> gPunctualLights : register(t0);

struct Output {
	nointerpolation PunctualLight mPunctualLight : PUNCTUAL_LIGHT;
};

[RootSignature(RS)]
Output main(in const Input input) {
	PunctualLight light = gPunctualLights[input.mVertexId];

	float4 lightPositionViewSpace = float4(light.mLightPosVAndRange.xyz, 1.0f);
	lightPositionViewSpace = mul(lightPositionViewSpace, gFrameCBuffer.mViewMatrix);

	Output output = (Output)0;
	output.mPunctualLight.mLightPosVAndRange = float4(lightPositionViewSpace.xyz, light.mLightPosVAndRange.w);
	output.mPunctualLight.mLightColorAndPower = light.mLightColorAndPower;
	return output;
}


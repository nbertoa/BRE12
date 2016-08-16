#include "../ShaderUtils/Lighting.hlsli"

#define MAX_NUM_LIGHTS 250

struct Input {
	uint mVertexId : SV_VertexID;
};

struct FrameConstants {
	float4x4 mV;
};
ConstantBuffer<FrameConstants> gFrameConstants : register(b0);

StructuredBuffer<PunctualLight> gPunctualLights : register(t0);

struct Output {
	nointerpolation PunctualLight mPunctualLight : PUNCTUAL_LIGHT;
};

Output main(in const Input input) {
	PunctualLight l = gPunctualLights[input.mVertexId];

	float4 lightPosV = float4(l.mLightPosVAndRange.xyz, 1.0f);
	lightPosV = mul(lightPosV, gFrameConstants.mV);

	Output output = (Output)0;
	output.mPunctualLight.mLightPosVAndRange = float4(lightPosV.xyz, l.mLightPosVAndRange.w);
	output.mPunctualLight.mLightColorAndPower = l.mLightColorAndPower;
	return output;
}


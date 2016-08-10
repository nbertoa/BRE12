#define MAX_NUM_LIGHTS 250

struct Input {
	uint mVertexId : SV_VertexID;
};

struct FrameConstants {
	float4x4 mV;
};
ConstantBuffer<FrameConstants> gFrameConstants : register(b0);

struct Light {
	float4 mLightPosVAndRange;
	float4 mLightColorAndPower;
	float4 mPad[14]; // Constant buffer must be aligned at 256 bytes
};
cbuffer CBufferPerFrame : register (b1) {
	Light mLights[MAX_NUM_LIGHTS];
};

struct Output {
	nointerpolation float4 mLightPosVAndRange : LIGHT_POSITION_AND_RANGE;
	nointerpolation float4 mLightColorAndPower : LIGHT_COLOR_AND_POWER;
};

Output main(in const Input input) {
	Light l = mLights[input.mVertexId];

	float4 lightPosV = float4(l.mLightPosVAndRange.xyz, 1.0f);
	lightPosV = mul(lightPosV, gFrameConstants.mV);

	Output output = (Output)0;
	output.mLightPosVAndRange = float4(lightPosV.xyz, l.mLightPosVAndRange.w);
	output.mLightColorAndPower = l.mLightColorAndPower;
	return output;
}


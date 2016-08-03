struct Input {
	uint mVertexId : SV_VertexID;
};

struct FrameConstants {
	float4x4 mV;
};
ConstantBuffer<FrameConstants> gFrameConstants : register(b0);

struct Output {
	nointerpolation float4 mLightPosVAndRange : LIGHT_POSITION_AND_RANGE;
	nointerpolation float4 mLightColorAndPower : LIGHT_COLOR_AND_POWER;
};

Output main(in const Input input) {
	float4 lightPosV = float4(0.0f, 0.0f, 0.0f, 1.0f);
	lightPosV = mul(lightPosV, gFrameConstants.mV);

	Output output = (Output)0;
	output.mLightPosVAndRange = float4(lightPosV.xyz, 800.0f);
	output.mLightColorAndPower = float4(1.0f, 1.0f, 1.0f, 4000.0f);
	return output;
}


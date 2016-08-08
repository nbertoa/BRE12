struct Input {
	uint mVertexId : SV_VertexID;
};

struct ObjConstants {
	float4x4 mW;
};
ConstantBuffer<ObjConstants> gObjConstants : register(b0);

struct FrameConstants {
	float4x4 mV;
};
ConstantBuffer<FrameConstants> gFrameConstants : register(b1);

struct Output {
	nointerpolation float4 mLightPosVAndRange : LIGHT_POSITION_AND_RANGE;
	nointerpolation float4 mLightColorAndPower : LIGHT_COLOR_AND_POWER;
};

Output main(in const Input input) {
	float4 lightPosV = float4(0.0f, 0.0f, 0.0f, 1.0f);
	float4x4 wv = mul(gObjConstants.mW, gFrameConstants.mV);	
	lightPosV = mul(lightPosV, wv);

	Output output = (Output)0;
	output.mLightPosVAndRange = float4(lightPosV.xyz, 30.0f);
	output.mLightColorAndPower = float4(1.0f, 1.0f, 1.0f, 4000.0f);
	return output;
}


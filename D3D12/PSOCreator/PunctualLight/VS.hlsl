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

	/*const float nearPlaneZ = 1.0f;
	const float lightMinZ = lightPosV.z - 8000.0f;
	const float lightMaxZ = lightPosV.z + 8000.0f;
	if (lightMinZ < nearPlaneZ && nearPlaneZ < lightMaxZ) {
		lightPosV.z = nearPlaneZ;
	}
	else {
		lightPosV.z = lightMinZ;
	}*/


	Output output = (Output)0;
	output.mLightPosVAndRange = float4(lightPosV.xyz, 8000.0f);
	output.mLightColorAndPower = float4(1.0f, 1.0f, 1.0f, 2000.0f);
	return output;
}


struct Input {
	uint mVertexId : SV_VertexID;
};

struct Output {
	nointerpolation float4 mLightPosVAndRange : LIGHT_POSITION_AND_RANGE;
	nointerpolation float4 mLightColorAndPower : LIGHT_COLOR_AND_POWER;
};

Output main(in const Input input) {
	// Add near/far plane limits check

	Output output = (Output)0;
	output.mLightPosVAndRange = float4(0.0f, 0.0f, 0.0f, 8000.0f);
	output.mLightColorAndPower = float4(1.0f, 1.0f, 1.0f, 2000.0f);
	return output;
}


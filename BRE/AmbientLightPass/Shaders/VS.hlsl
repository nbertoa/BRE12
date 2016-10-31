struct Input {
	float3 mPosH : POSITION;
	float3 mNormalO : NORMAL;
	float3 mTangentO : TANGENT;
	float2 mTexCoordO : TEXCOORD;
};

struct Output {
	float4 mPosH : SV_POSITION;
};

Output main(in const Input input) {
	Output output;

	output.mPosH = float4(input.mPosH, 1.0f);
	
	return output;
}
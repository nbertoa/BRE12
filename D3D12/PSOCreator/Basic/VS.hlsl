struct Input {
	float3 mPosO : POSITION;
	float3 mNormalO : NORMAL;
	float3 mTangentO : TANGENT;
	float2 mTexCoordO : TEXCOORD;
};

struct ObjConstants {
	float4x4 mW;
};
ConstantBuffer<ObjConstants> gObjConstants : register(b0);

struct FrameConstants {
	float4x4 mV;
	float4x4 mP;
};
ConstantBuffer<FrameConstants> gFrameConstants : register(b1);

struct Output {	
	float4 mPosH : SV_POSITION;
	float3 mPosV : POS_VIEW;
	float3 mNormalV : NORMAL_VIEW;
};

Output main(in const Input input) {
	float4x4 wv = mul(gObjConstants.mW, gFrameConstants.mV);

	Output output;
	output.mPosV = mul(float4(input.mPosO, 1.0f), wv).xyz;
	output.mNormalV = mul(float4(input.mNormalO, 0.0f), wv).xyz;
	output.mPosH = mul(float4(output.mPosV, 1.0f), gFrameConstants.mP);

	return output;
}
#include "../ShaderUtils/CBuffers.hlsli"

struct Input {
	float3 mPosO : POSITION;
	float3 mNormalO : NORMAL;
	float3 mTangentO : TANGENT;
	float2 mTexCoordO : TEXCOORD;
};

ConstantBuffer<ObjectCBuffer> gObjCBuffer : register(b0);
ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b1);

struct Output {
	float4 mPosH : SV_POSITION;
	float3 mPosV : POS_VIEW;
	float3 mNormalV : NORMAL_VIEW;
	float2 mTexCoordO : TEXCOORD;
};

Output main(in const Input input) {
	const float4x4 wv = mul(gObjCBuffer.mW, gFrameCBuffer.mV);

	Output output;
	output.mPosV = mul(float4(input.mPosO, 1.0f), wv).xyz;
	output.mNormalV = mul(float4(input.mNormalO, 0.0f), wv).xyz;
	output.mPosH = mul(float4(output.mPosV, 1.0f), gFrameCBuffer.mP);
	output.mTexCoordO = gObjCBuffer.mTexTransform * input.mTexCoordO;

	return output;
}
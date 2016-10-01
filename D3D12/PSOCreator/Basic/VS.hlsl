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
	float3 mPosW : POS_WORLD;
	float3 mPosV : POS_VIEW;
	float3 mNormalW : NORMAL_WORLD;
	float3 mNormalV : NORMAL_VIEW;
};

Output main(in const Input input) {
	Output output;

	output.mPosW = mul(float4(input.mPosO, 1.0f), gObjCBuffer.mW).xyz;
	output.mPosV = mul(float4(output.mPosW, 1.0f), gFrameCBuffer.mV).xyz;

	output.mNormalW = mul(float4(input.mNormalO, 0.0f), gObjCBuffer.mW).xyz;
	output.mNormalV = mul(float4(output.mNormalW, 0.0f), gFrameCBuffer.mV).xyz;

	output.mPosH = mul(float4(output.mPosV, 1.0f), gFrameCBuffer.mP);

	return output;
}
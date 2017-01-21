#include <ShaderUtils/CBuffers.hlsli>

#include "RS.hlsl"

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
	float3 mTangentW : TANGENT_WORLD;
	float3 mTangentV : TANGENT_VIEW;
	float3 mBinormalW : BINORMAL_WORLD;
	float3 mBinormalV : BINORMAL_VIEW;
	float2 mTexCoordO : TEXCOORD;
};

Output main(in const Input input) {
	Output output;
	output.mPosW = mul(float4(input.mPosO, 1.0f), gObjCBuffer.mW).xyz;
	output.mPosV = mul(float4(output.mPosW, 1.0f), gFrameCBuffer.mV).xyz;
	output.mPosH = mul(float4(output.mPosV, 1.0f), gFrameCBuffer.mP);

	output.mTexCoordO = gObjCBuffer.mTexTransform * input.mTexCoordO;

	output.mNormalW = mul(float4(input.mNormalO, 0.0f), gObjCBuffer.mW).xyz;
	output.mNormalV = mul(float4(output.mNormalW, 0.0f), gFrameCBuffer.mV).xyz;

	output.mTangentW = mul(float4(input.mTangentO, 0.0f), gObjCBuffer.mW).xyz;
	output.mTangentV = mul(float4(output.mTangentW, 0.0f), gFrameCBuffer.mV).xyz;
	
	output.mBinormalW = normalize(cross(output.mNormalW, output.mTangentW));
	output.mBinormalV = normalize(cross(output.mNormalV, output.mTangentV));

	return output;
}
#include <ShaderUtils/CBuffers.hlsli>

#include "RS.hlsl"

struct Input {
	float3 mPositionObjectSpace : POSITION;
	float3 mNormalObjectSpace : NORMAL;
	float3 mTangentObjectSpace : TANGENT;
	float2 mUV : TEXCOORD;
};

ConstantBuffer<ObjectCBuffer> gObjCBuffer : register(b0);
ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b1);

struct Output {
	float4 mPositionClipSpace : SV_POSITION;
	float3 mPositionWorldSpace : POS_WORLD;
	float3 mPositionViewSpace : POS_VIEW;
	float3 mNormalWorldSpace : NORMAL_WORLD;
	float3 mNormalViewSpace : NORMAL_VIEW;
	float3 mTangentWorldSpace : TANGENT_WORLD;
	float3 mTangentViewSpace : TANGENT_VIEW;
	float3 mBinormalWorldSpace : BINORMAL_WORLD;
	float3 mBinormalViewSpace : BINORMAL_VIEW;
	float2 mUV : TEXCOORD;
};

[RootSignature(RS)]
Output main(in const Input input) {
	Output output;
	output.mPositionWorldSpace = mul(float4(input.mPositionObjectSpace, 1.0f), gObjCBuffer.mWorldMatrix).xyz;
	output.mPositionViewSpace = mul(float4(output.mPositionWorldSpace, 1.0f), gFrameCBuffer.mViewMatrix).xyz;
	output.mPositionClipSpace = mul(float4(output.mPositionViewSpace, 1.0f), gFrameCBuffer.mProjectionMatrix);

	output.mUV = gObjCBuffer.mTexTransform * input.mUV;

	output.mNormalWorldSpace = mul(float4(input.mNormalObjectSpace, 0.0f), gObjCBuffer.mWorldMatrix).xyz;
	output.mNormalViewSpace = mul(float4(output.mNormalWorldSpace, 0.0f), gFrameCBuffer.mViewMatrix).xyz;

	output.mTangentWorldSpace = mul(float4(input.mTangentObjectSpace, 0.0f), gObjCBuffer.mWorldMatrix).xyz;
	output.mTangentViewSpace = mul(float4(output.mTangentWorldSpace, 0.0f), gFrameCBuffer.mViewMatrix).xyz;
	
	output.mBinormalWorldSpace = normalize(cross(output.mNormalWorldSpace, output.mTangentWorldSpace));
	output.mBinormalViewSpace = normalize(cross(output.mNormalViewSpace, output.mTangentViewSpace));

	return output;
}
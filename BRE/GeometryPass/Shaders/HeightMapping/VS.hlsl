#include <ShaderUtils/CBuffers.hlsli>

#include "RS.hlsl"

#define MIN_TESS_DISTANCE 25.0f
#define MAX_TESS_DISTANCE 1.0f
#define MIN_TESS_FACTOR 1.0f
#define MAX_TESS_FACTOR 5.0f

struct Input {
	float3 mPositionObjectSpace : POSITION;
	float3 mNormalObjectSpace : NORMAL;
	float3 mTangentObjectSpace : TANGENT;
	float2 mUV : TEXCOORD;
};

ConstantBuffer<ObjectCBuffer> gObjCBuffer : register(b0);
ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b1);

struct Output {
	float3 mPositionWorldSpace : POS_WORLD;	
	float3 mNormalWorldSpace : NORMAL_WORLD;
	float3 mTangentWorldSpace : TANGENT_WORLD;
	float2 mUV : TEXCOORD0;
	float mTessellationFactor : TESS;
};

[RootSignature(RS)]
Output main(in const Input input) {
	Output output;

	output.mPositionWorldSpace = mul(float4(input.mPositionObjectSpace, 1.0f), gObjCBuffer.mWorldMatrix).xyz;

	output.mNormalWorldSpace = mul(float4(input.mNormalObjectSpace, 0.0f), gObjCBuffer.mInverseTransposeWorldMatrix).xyz;

	output.mTangentWorldSpace = mul(float4(input.mTangentObjectSpace, 0.0f), gObjCBuffer.mWorldMatrix).xyz;

	output.mUV = gObjCBuffer.mTexTransform * input.mUV;
		
	// Normalized tessellation factor. 
	// The tessellation is 
	//   0 if d >= MIN_TESS_DISTANCE and
	//   1 if d <= MAX_TESS_DISTANCE.  
	const float distance = length(output.mPositionWorldSpace - gFrameCBuffer.mEyePositionWorldSpace.xyz);
	const float tessellationFactor = saturate((MIN_TESS_DISTANCE - distance) / (MIN_TESS_DISTANCE - MAX_TESS_DISTANCE));

	// Rescale [0,1] --> [MIN_TESS_FACTOR, MAX_TESS_FACTOR].
	output.mTessellationFactor = MIN_TESS_FACTOR + tessellationFactor * (MAX_TESS_FACTOR - MIN_TESS_FACTOR);

	return output;
}
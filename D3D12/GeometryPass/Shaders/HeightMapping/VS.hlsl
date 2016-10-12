#include <ShaderUtils/CBuffers.hlsli>

#define MIN_TESS_DISTANCE 25.0f
#define MAX_TESS_DISTANCE 1.0f
#define MIN_TESS_FACTOR 1.0f
#define MAX_TESS_FACTOR 5.0f

struct Input {
	float3 mPosO : POSITION;
	float3 mNormalO : NORMAL;
	float3 mTangentO : TANGENT;
	float2 mTexCoordO : TEXCOORD;
};

ConstantBuffer<ObjectCBuffer> gObjCBuffer : register(b0);
ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b1);

struct Output {
	float3 mPosW : POS_WORLD;	
	float3 mNormalW : NORMAL_WORLD;
	float3 mTangentW : TANGENT_WORLD;
	float2 mTexCoordO : TEXCOORD0;
	float mTessFactor : TESS;
};

Output main(in const Input input) {
	Output output;

	output.mPosW = mul(float4(input.mPosO, 1.0f), gObjCBuffer.mW).xyz;

	output.mNormalW = mul(float4(input.mNormalO, 0.0f), gObjCBuffer.mW).xyz;

	output.mTangentW = mul(float4(input.mTangentO, 0.0f), gObjCBuffer.mW).xyz;

	output.mTexCoordO = gObjCBuffer.mTexTransform * input.mTexCoordO;
		

	// Normalized tessellation factor. 
	// The tessellation is 
	//   0 if d >= MIN_TESS_DISTANCE and
	//   1 if d <= MAX_TESS_DISTANCE.  
	const float d = length(output.mPosW - gFrameCBuffer.mEyePosW);
	const float tess = saturate((MIN_TESS_DISTANCE - d) / (MIN_TESS_DISTANCE - MAX_TESS_DISTANCE));

	// Rescale [0,1] --> [MIN_TESS_FACTOR, MAX_TESS_FACTOR].
	output.mTessFactor = MIN_TESS_FACTOR + tess * (MAX_TESS_FACTOR - MIN_TESS_FACTOR);

	return output;
}
#include "../ShaderUtils/CBuffers.hlsli"

#define NUM_PATCH_POINTS 3
#define HEIGHT_SCALE 0.07f

struct HullShaderConstantOutput {
	float mEdgeFactors[3] : SV_TessFactor;
	float mInsideFactors : SV_InsideTessFactor;
};

struct Input {
	float3 mPosW : POS_WORLD;
	float3 mNormalW : NORMAL_WORLD;
	float3 mTangentW : TANGENT_WORLD;
	float2 mTexCoordO : TEXCOORD0;
};

ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b0);

struct Output {
	float4 mPosH : SV_Position;
	float3 mPosW : POS_WORLD;
	float3 mPosV : POS_VIEW;
	float3 mNormalW : NORMAL_WORLD;	
	float3 mNormalV : NORMAL_VIEW;
	float3 mTangentW : TANGENT_WORLD;
	float3 mTangentV : TANGENT_VIEW;
	float3 mBinormalW : BINORMAL_WORLD;
	float3 mBinormalV : BINORMAL_VIEW;
	float2 mTexCoordO : TEXCOORD0;
};

SamplerState TexSampler : register (s0);
Texture2D HeightTexture : register (t0);

[domain("tri")]
Output main(const HullShaderConstantOutput HSConstantOutput, const float3 uvw : SV_DomainLocation, const OutputPatch <Input, NUM_PATCH_POINTS> patch) {
	Output output = (Output)0;

	output.mTexCoordO = uvw.x * patch[0].mTexCoordO + uvw.y * patch[1].mTexCoordO + uvw.z * patch[2].mTexCoordO;

	const float3 normalW = normalize(uvw.x * patch[0].mNormalW + uvw.y * patch[1].mNormalW + uvw.z * patch[2].mNormalW);
	output.mNormalW = normalize(normalW);
	output.mNormalV = normalize(mul(float4(output.mNormalW, 0.0f), gFrameCBuffer.mV).xyz);

	float3 posW = uvw.x * patch[0].mPosW + uvw.y * patch[1].mPosW + uvw.z * patch[2].mPosW;
	float3 posV = mul(float4(posW, 1.0f), gFrameCBuffer.mV).xyz;

	// Choose the mipmap level based on distance to the eye; specifically, choose the next miplevel every MipInterval units, and clamp the miplevel in [0,6].
	const float MipInterval = 20.0f;
	const float mipLevel = clamp((length(posV) - MipInterval) / MipInterval, 0.0f, 6.0f);
	const float height = HeightTexture.SampleLevel(TexSampler, output.mTexCoordO, mipLevel).x;
	const float displacement = (HEIGHT_SCALE * (height - 1));

	// Offset vertex along normal
	posW += output.mNormalW * displacement;
	posV += output.mNormalV * displacement;
	
	output.mTangentW = normalize(uvw.x * patch[0].mTangentW + uvw.y * patch[1].mTangentW + uvw.z * patch[2].mTangentW);
	output.mTangentV = normalize(mul(float4(output.mTangentW, 0.0f), gFrameCBuffer.mV)).xyz;

	output.mBinormalW = normalize(cross(output.mNormalW, output.mTangentW));
	output.mBinormalV = normalize(cross(output.mNormalV, output.mTangentV));
	
	output.mPosW = posW;
	output.mPosV = posV;
	output.mPosH = mul(float4(posV, 1.0f), gFrameCBuffer.mP);

	return output;
}
#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Material.hlsli>
#include <ShaderUtils/Utils.hlsli>

struct Input {
	float4 mPosH : SV_POSITION;
	float3 mPosW : POS_WORLD;
	float3 mPosV : POS_VIEW;
	float3 mNormalW : NORMAL_WORLD;
	float3 mNormalV : NORMAL_VIEW;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b1);

SamplerState TexSampler : register (s0);
TextureCube DiffuseCubeMap : register(t0);
TextureCube SpecularCubeMap : register(t1);

struct Output {	
	float4 mNormal_Smoothness : SV_Target0;	
	float4 mBaseColor_MetalMask : SV_Target1;
	float4 mDiffuseReflection : SV_Target2;
	float4 mSpecularReflection : SV_Target3;
	float mDepth : SV_Target4;
};

Output main(const in Input input) {
	Output output = (Output)0;

	// Normal (encoded in view space)
	const float3 normal = normalize(input.mNormalV);
	output.mNormal_Smoothness.xy = Encode(normal);

	// Metal mask
	output.mBaseColor_MetalMask = gMaterial.mBaseColor_MetalMask;

	// Smoothness
	output.mNormal_Smoothness.z = gMaterial.mSmoothness;

	// Depth (view space)
	output.mDepth = length(input.mPosV);

	// Compute diffuse reflection.
	output.mDiffuseReflection.rgb = DiffuseCubeMap.Sample(TexSampler, input.mNormalW).rgb;

	// Compute specular reflection.
	const float3 incidentVecW = input.mPosW - gFrameCBuffer.mEyePosW.xyz;
	const float3 reflectionVecW = reflect(incidentVecW, input.mNormalW);
	// Our cube map has 10 mip map levels (0 - 9) based on smoothness
	const uint mipmap = (1.0f - gMaterial.mSmoothness) * 9.0f;
	output.mSpecularReflection.rgb = SpecularCubeMap.SampleLevel(TexSampler, reflectionVecW, mipmap).rgb;
		
	return output;
}
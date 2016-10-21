#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Material.hlsli>
#include <ShaderUtils/Utils.hlsli>

struct Input {
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

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b1);

SamplerState TexSampler : register (s0);
Texture2D DiffuseTexture : register (t0);
Texture2D NormalTexture : register (t1);
TextureCube DiffuseCubeMap : register(t2);
TextureCube SpecularCubeMap : register(t3);

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
	const float3 sampledNormal = normalize(UnmapNormal(NormalTexture.Sample(TexSampler, input.mTexCoordO).xyz));
	const float3x3 tbnW = float3x3(normalize(input.mTangentW), normalize(input.mBinormalW), normalize(input.mNormalW));
	const float3 normalW = normalize(mul(sampledNormal, tbnW));
	const float3x3 tbnV = float3x3(normalize(input.mTangentV), normalize(input.mBinormalV), normalize(input.mNormalV));
	output.mNormal_Smoothness.xy = Encode(mul(sampledNormal, tbnV));

	// Base color and metal mask
	const float3 diffuseColor = DiffuseTexture.Sample(TexSampler, input.mTexCoordO).rgb;
	output.mBaseColor_MetalMask = float4(gMaterial.mBaseColor_MetalMask.xyz * diffuseColor, gMaterial.mBaseColor_MetalMask.w);

	// Smoothness
	output.mNormal_Smoothness.z = gMaterial.mSmoothness;

	// Depth (view space)
	output.mDepth = length(input.mPosV);

	// Compute diffuse reflection.
	output.mDiffuseReflection.rgb = DiffuseCubeMap.SampleLevel(TexSampler, input.mNormalW, lerp(9, 0, gMaterial.mSmoothness)).rgb;

	// Compute specular reflection.
	const float3 incidentVecW = input.mPosW - gFrameCBuffer.mEyePosW.xyz;
	const float3 reflectionVecW = reflect(incidentVecW, input.mNormalW);
	output.mSpecularReflection.rgb = SpecularCubeMap.SampleLevel(TexSampler, reflectionVecW, lerp(9, 0, gMaterial.mSmoothness)).rgb;

	return output;
}
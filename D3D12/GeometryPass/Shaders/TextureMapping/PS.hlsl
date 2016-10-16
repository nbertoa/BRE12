#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Material.hlsli>
#include <ShaderUtils/Utils.hlsli>

struct Input {
	float4 mPosH : SV_POSITION;
	float3 mPosW : POS_WORLD;
	float3 mPosV : POS_VIEW;
	float3 mNormalW : NORMAL_WORLD;
	float3 mNormalV : NORMAL_VIEW;
	float2 mTexCoordO : TEXCOORD;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<ImmutableCBuffer> gImmutableCBuffer : register(b1);
ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b2);

SamplerState TexSampler : register (s0);
Texture2D DiffuseTexture : register (t0);
TextureCube CubeMap : register(t1);

struct Output {
	float4 mNormalV_Smoothness_DepthV : SV_Target0;
	float4 mBaseColor_MetalMask : SV_Target1;
	float4 mDiffuseReflection : SV_Target2;
	float4 mSpecularReflection : SV_Target3;
};

Output main(const in Input input) {
	Output output = (Output)0;

	// Normal (encoded in view space)
	const float3 normal = normalize(input.mNormalV);
	output.mNormalV_Smoothness_DepthV.xy = Encode(normal);

	// Base color and metal mask
	const float3 diffuseColor = DiffuseTexture.Sample(TexSampler, input.mTexCoordO).rgb;
	output.mBaseColor_MetalMask = float4(gMaterial.mBaseColor_MetalMask.xyz * diffuseColor, gMaterial.mBaseColor_MetalMask.w);

	// Smoothness
	output.mNormalV_Smoothness_DepthV.z = gMaterial.mSmoothness;

	// Depth (view space)
	output.mNormalV_Smoothness_DepthV.w = input.mPosV.z / gImmutableCBuffer.mNearZ_FarZ_ScreenW_ScreenH.y;

	// Compute diffuse reflection.
	output.mDiffuseReflection.rgb = CubeMap.Sample(TexSampler, input.mNormalW).rgb;

	// Compute specular reflection.
	const float3 incidentVecW = input.mPosW - gFrameCBuffer.mEyePosW;
	const float3 reflectionVecW = reflect(incidentVecW, input.mNormalW);
	output.mSpecularReflection.rgb = CubeMap.Sample(TexSampler, reflectionVecW).rgb;

	return output;
}
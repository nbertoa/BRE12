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
Texture2D NormalTexture : register (t0);

struct Output {
	float4 mNormal_Smoothness : SV_Target0;
	float4 mBaseColor_MetalMask : SV_Target1;
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
	output.mBaseColor_MetalMask = gMaterial.mBaseColor_MetalMask;

	// Smoothness
	output.mNormal_Smoothness.z = gMaterial.mSmoothness;

	return output;
}
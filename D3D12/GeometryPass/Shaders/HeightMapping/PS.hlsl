#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Material.hlsli>
#include <ShaderUtils/Utils.hlsli>

struct Input {
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

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<ImmutableCBuffer> gImmutableCBuffer : register(b1);
ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b2);

SamplerState TexSampler : register (s0);
Texture2D DiffuseTexture : register (t0);
Texture2D NormalTexture : register (t1);
TextureCube CubeMap : register(t2);

struct Output {
	float4 mNormalV_Smoothness_DepthV : SV_Target0;
	float4 mBaseColor_MetalMask : SV_Target1;
	float4 mSpecularReflection : SV_Target2;
};

Output main(const in Input input) {
	Output output = (Output)0;

	// Normal (encoded in view space) 
	const float3 sampledNormal = normalize(UnmapNormal(NormalTexture.Sample(TexSampler, input.mTexCoordO).xyz));
	const float3x3 tbnW = float3x3(normalize(input.mTangentW), normalize(input.mBinormalW), normalize(input.mNormalW));
	const float3 normalW = mul(sampledNormal, tbnW);
	const float3x3 tbnV = float3x3(normalize(input.mTangentV), normalize(input.mBinormalV), normalize(input.mNormalV));
	output.mNormalV_Smoothness_DepthV.xy = Encode(mul(sampledNormal, tbnV));

	// Base color and metal mask
	const float3 diffuseColor = DiffuseTexture.Sample(TexSampler, input.mTexCoordO).rgb;
	output.mBaseColor_MetalMask = float4(gMaterial.mBaseColor_MetalMask.xyz * diffuseColor, gMaterial.mBaseColor_MetalMask.w);

	// Smoothness
	output.mNormalV_Smoothness_DepthV.z = gMaterial.mSmoothness;

	// Depth (view space)
	output.mNormalV_Smoothness_DepthV.w = input.mPosV.z / gImmutableCBuffer.mNearZ_FarZ_ScreenW_ScreenH.y;

	// Specular reflection
	const float3 toEyeW = gFrameCBuffer.mEyePosW - input.mPosW;
	const float3 r = reflect(-toEyeW, normalW);
	output.mSpecularReflection.rgb = CubeMap.Sample(TexSampler, r).rgb;

	return output;
}
#include "../ShaderUtils/CBuffers.hlsli"
#include "../ShaderUtils/Material.hlsli"
#include "../ShaderUtils/Utils.hlsli"

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
	float4 mNormalV_Smoothness_Depth : SV_Target0;
	float4 mBaseColor_MetalMask : SV_Target1;
	float4 mSpecularReflection : SV_Target2;
};

Output main(const in Input input) {
	Output output = (Output)0;
	const float3 normal = normalize(input.mNormalV);
	output.mNormalV_Smoothness_Depth.xy = Encode(normal);

	const float3 diffuseColor = DiffuseTexture.Sample(TexSampler, input.mTexCoordO).rgb;
	output.mBaseColor_MetalMask = float4(gMaterial.mBaseColor_MetalMask.xyz * diffuseColor, gMaterial.mBaseColor_MetalMask.w);

	output.mNormalV_Smoothness_Depth.z = gMaterial.mSmoothness;
	output.mNormalV_Smoothness_Depth.w = input.mPosV.z / gImmutableCBuffer.mNearZ_FarZ_ScreenW_ScreenH.y;

	const float3 toEyeW = gFrameCBuffer.mEyePosW - input.mPosW;
	const float3 r = reflect(-toEyeW, input.mNormalW);
	output.mSpecularReflection.rgb = CubeMap.Sample(TexSampler, r).rgb;

	return output;
}
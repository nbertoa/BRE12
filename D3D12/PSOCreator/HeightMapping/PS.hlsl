#include "../ShaderUtils/CBuffers.hlsli"
#include "../ShaderUtils/Material.hlsli"
#include "../ShaderUtils/Utils.hlsli"

struct Input {
	float4 mPosH : SV_POSITION;
	float3 mPosV : POS_VIEW;
	float3 mNormalV : NORMAL;
	float3 mTangentV : TANGENT;
	float3 mBinormalV : BINORMAL;
	float2 mTexCoordO : TEXCOORD;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<ImmutableCBuffer> gImmutableCBuffer : register(b1);

SamplerState TexSampler : register (s0);
Texture2D DiffuseTexture : register (t0);
Texture2D NormalTexture : register (t1);

struct Output {
	float4 mNormalV_Smoothness_Depth : SV_Target0;
	float4 mBaseColor_MetalMask : SV_Target1;
};

Output main(const in Input input) {
	Output output = (Output)0;

	const float3 sampledNormal = normalize(UnmapNormal(NormalTexture.Sample(TexSampler, input.mTexCoordO).xyz));
	const float3x3 tbn = float3x3(normalize(input.mTangentV), normalize(input.mBinormalV), normalize(input.mNormalV));
	output.mNormalV_Smoothness_Depth.xy = Encode(mul(sampledNormal, tbn));

	const float3 diffuseColor = DiffuseTexture.Sample(TexSampler, input.mTexCoordO).rgb;
	output.mBaseColor_MetalMask = float4(gMaterial.mBaseColor_MetalMask.xyz * diffuseColor, gMaterial.mBaseColor_MetalMask.w);

	output.mNormalV_Smoothness_Depth.z = gMaterial.mSmoothness;

	output.mNormalV_Smoothness_Depth.w = input.mPosV.z / gImmutableCBuffer.mNearZ_FarZ_ScreenW_ScreenH.y;

	return output;
}
#include "../ShaderUtils/CBuffers.hlsli"
#include "../ShaderUtils/Material.hlsli"
#include "../ShaderUtils/Utils.hlsli"

struct Input {
	float4 mPosH : SV_POSITION;
	float3 mPosV : POS_VIEW;
	float3 mNormalV : NORMAL_VIEW;
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
	float2 mNormalV : SV_Target0;
	float4 mBaseColor_MetalMask : SV_Target1;
	float mSmoothness : SV_Target2;
	float mDepthV : SV_Target3;
};

Output main(const in Input input) {
	Output output = (Output)0;

	const float3 sampledNormal = normalize(UnmapNormal(NormalTexture.Sample(TexSampler, input.mTexCoordO).xyz));
	const float3x3 tbn = float3x3(normalize(input.mTangentV), normalize(input.mBinormalV), normalize(input.mNormalV));
	output.mNormalV = Encode(mul(sampledNormal, tbn));

	const float3 diffuseColor = DiffuseTexture.Sample(TexSampler, input.mTexCoordO).rgb;
	output.mBaseColor_MetalMask = float4(gMaterial.mBaseColor_MetalMask.xyz * diffuseColor, gMaterial.mBaseColor_MetalMask.w);

	output.mSmoothness = gMaterial.mSmoothness;

	output.mDepthV = input.mPosV.z / gImmutableCBuffer.mNearZ_FarZ_ScreenW_ScreenH.y;

	return output;
}
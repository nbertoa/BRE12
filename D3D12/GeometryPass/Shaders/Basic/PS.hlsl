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

struct Output {	
	float4 mNormal_Smoothness : SV_Target0;	
	float4 mBaseColor_MetalMask : SV_Target1;
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
		
	return output;
}
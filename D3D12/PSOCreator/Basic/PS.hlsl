#include "../ShaderUtils/CBuffers.hlsli"
#include "../ShaderUtils/Material.hlsli"
#include "../ShaderUtils/Utils.hlsli"

struct Input {
	float4 mPosH : SV_POSITION;
	float3 mPosV : POS_VIEW;
	float3 mNormalV : NORMAL_VIEW;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<ImmutableCBuffer> gImmutableCBuffer : register(b1);

struct Output {	
	float2 mNormalV : SV_Target0;	
	float4 mBaseColor_MetalMask : SV_Target1;
	float2 mReflectance_Smoothness : SV_Target2;
	float mDepthV : SV_Target3;
};

Output main(const in Input input) {
	Output output = (Output)0;
	float3 normal = normalize(input.mNormalV);
	output.mNormalV = Encode(normal);
	output.mBaseColor_MetalMask = gMaterial.mBaseColor_MetalMask;
	output.mReflectance_Smoothness = gMaterial.mReflectance_Smoothness;
	output.mDepthV = input.mPosV.z / gImmutableCBuffer.mNearZ_FarZ_ScreenW_ScreenH.y;
	
	return output;
}
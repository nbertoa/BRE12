#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/MaterialProperties.hlsli>
#include <ShaderUtils/Utils.hlsli>

#include "RS.hlsl"

struct Input {
	float4 mPositionClipSpace : SV_POSITION;
	float3 mPositionWorldSpace : POS_WORLD;
	float3 mPositionViewSpace : POS_VIEW;
	float3 mNormalWorldSpace : NORMAL_WORLD;
	float3 mNormalViewSpace : NORMAL_VIEW;
};

ConstantBuffer<MaterialProperties> gMaterialPropertiesCBuffer : register(b0);
ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b1);

struct Output {	
	float4 mNormal_Smoothness : SV_Target0;	
	float4 mBaseColor_MetalMask : SV_Target1;
};

[RootSignature(RS)]
Output main(const in Input input) {
	Output output = (Output)0;

	// Normal (encoded in view space)
	const float3 normal = normalize(input.mNormalViewSpace);
	output.mNormal_Smoothness.xy = Encode(normal);

	// Metal mask
	output.mBaseColor_MetalMask = gMaterialPropertiesCBuffer.mBaseColor_MetalMask;

	// Smoothness
	output.mNormal_Smoothness.z = gMaterialPropertiesCBuffer.mSmoothness;
		
	return output;
}
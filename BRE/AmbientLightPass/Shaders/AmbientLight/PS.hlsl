#include "RS.hlsl"

#define AMBIENT_FACTOR 0.04f

struct Input {
	float4 mPosH : SV_POSITION;
};

Texture2D<float4> BaseColor_MetalMask : register (t0);
Texture2D<float> AmbientAccessibility : register (t1);

struct Output {
	float4 mColor : SV_Target0;
};

[RootSignature(RS)]
Output main(const in Input input){
	Output output = (Output)0;

	// Get base color
	const int3 screenCoord = int3(input.mPosH.xy, 0);
	const float3 baseColor = BaseColor_MetalMask.Load(screenCoord).xyz;

	// Get ambient accessibility (1.0f - ambient occlussion factor)
	const float accessibility = AmbientAccessibility.Load(screenCoord);

	output.mColor = float4(baseColor * AMBIENT_FACTOR/* * accessibility*/, 1.0f);
	//output.mColor = float4(accessibility, accessibility, accessibility, 1.0f);
	
	return output;
}
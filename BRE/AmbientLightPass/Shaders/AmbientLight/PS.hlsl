#include "RS.hlsl"

#define AMBIENT_FACTOR 0.4f

//#define DEBUG_AMBIENT_ACCESIBILITY

struct Input {
	float4 mPositionNDC : SV_POSITION;
};

Texture2D<float4> BaseColor_MetalMaskTexture : register (t0);
Texture2D<float> AmbientAccessibility : register (t1);

struct Output {
	float4 mColor : SV_Target0;
};

[RootSignature(RS)]
Output main(const in Input input){
	Output output = (Output)0;

	const int3 fragmentScreenSpace = int3(input.mPositionNDC.xy, 0);

	const float3 baseColor = BaseColor_MetalMaskTexture.Load(fragmentScreenSpace).xyz;

	// Ambient accessibility (1.0f - ambient occlussion factor)
	const float ambientAccessibility = AmbientAccessibility.Load(fragmentScreenSpace);

	output.mColor = float4(baseColor * AMBIENT_FACTOR/* * ambientAccessibility*/, 1.0f);

#ifdef DEBUG_AMBIENT_ACCESIBILITY
	output.mColor = float4(ambientAccessibility, ambientAccessibility, ambientAccessibility, 1.0f);
#endif
	
	return output;
}
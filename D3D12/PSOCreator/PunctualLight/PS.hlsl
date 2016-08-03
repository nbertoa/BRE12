#include "../Lighting.hlsli"

#define BRDF_FROSTBITE 

struct Input {
	float4 mPosH : SV_POSITION;
	nointerpolation float4 mLightPosVAndRange : LIGHT_POSITION_AND_RANGE;
	nointerpolation float4 mLightColorAndPower : LIGHT_COLOR_AND_POWER;
};

Texture2D NormalV : register (t0);
Texture2D PositionV : register (t1);
Texture2D BaseColor_MetalMask : register (t2);
Texture2D Reflectance_Smoothness : register (t3);

struct Output {
	float4 mColor : SV_Target0;
};

Output main(const in Input input) {
	Output output = (Output)0;

	// Determine our indices for sampling the texture based on the current
	// screen position
	const int3 sampleIndices = int3(input.mPosH.xy, 0);

	PunctualLight light;
	light.mPosV = input.mLightPosVAndRange.xyz;
	light.mRange = input.mLightPosVAndRange.w;
	light.mColor = input.mLightColorAndPower.xyz;
	light.mPower = input.mLightColorAndPower.w;

	const float3 geomPosV = PositionV.Load(sampleIndices).xyz;
	const float3 luminance = computeLuminance(light, geomPosV);

	const float4 baseColor_metalmask = BaseColor_MetalMask.Load(sampleIndices);
	const float4 reflectance_smoothness = Reflectance_Smoothness.Load(sampleIndices);
	const float3 lightDirV = normalize(light.mPosV - geomPosV);
	const float3 normalV = normalize(NormalV.Load(sampleIndices).xyz);
	// As we are working at view space, we do not need camera position to 
	// compute vector from geometry position to camera.
	const float3 viewV = normalize(-geomPosV);

	float3 illuminance;
#ifdef BRDF_FROSTBITE
	illuminance = brdf_FrostBite(normalV, viewV, lightDirV, baseColor_metalmask.xyz, reflectance_smoothness.w, reflectance_smoothness.xyz, baseColor_metalmask.w);
#else
	illuminance = brdf_CookTorrance(normalV, viewV, lightDirV, baseColor_metalmask.xyz, reflectance_smoothness.w, reflectance_smoothness.xyz, baseColor_metalmask.w);
#endif
	output.mColor = float4(luminance * illuminance, 1.0f);

	return output;
}
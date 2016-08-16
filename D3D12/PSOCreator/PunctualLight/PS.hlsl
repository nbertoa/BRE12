#include "../ShaderUtils/Lighting.hlsli"

#define BRDF_FROSTBITE_ILLUMINANCE 
#define BRDF_FROSTBITE_LUMINANCE

struct Input {
	float4 mPosH : SV_POSITION;
	nointerpolation PunctualLight mPunctualLight : PUNCTUAL_LIGHT;
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

	const int3 texCoord = int3(input.mPosH.xy, 0);

	PunctualLight light = input.mPunctualLight;

	const float3 normalV = normalize(NormalV.Load(texCoord).xyz);
	const float3 geomPosV = PositionV.Load(texCoord).xyz;

	const float4 baseColor_metalmask = BaseColor_MetalMask.Load(texCoord);
	const float4 reflectance_smoothness = Reflectance_Smoothness.Load(texCoord);
	const float3 lightDirV = normalize(light.mLightPosVAndRange.xyz - geomPosV);
	// As we are working at view space, we do not need camera position to 
	// compute vector from geometry position to camera.
	const float3 viewV = normalize(-geomPosV);

	float3 luminance;
#ifdef BRDF_FROSTBITE_LUMINANCE
	luminance = computePunctualLightFrostbiteLuminance(light, geomPosV, normalV);
#else
	luminance = computePunctualLightDirectLuminance(light, geomPosV, normalV, 0.0000f);
#endif

	// Discard if luminance does not contribute any light.
	uint cond = dot(luminance, 1.0f) == 0;
	clip(cond * -1 + (1 - cond));

	float3 illuminance;
#ifdef BRDF_FROSTBITE_ILLUMINANCE
	illuminance = brdf_FrostBite(normalV, viewV, lightDirV, baseColor_metalmask.xyz, reflectance_smoothness.w, reflectance_smoothness.xyz, baseColor_metalmask.w);
#else
	illuminance = brdf_CookTorrance(normalV, viewV, lightDirV, baseColor_metalmask.xyz, reflectance_smoothness.w, reflectance_smoothness.xyz, baseColor_metalmask.w);
#endif

	// Discard if illuminance does not contribute any light.
	cond = dot(illuminance, 1.0f) == 0;
	clip(cond * -1 + (1 - cond));

	output.mColor = float4(luminance * illuminance, 1.0f);

	return output;
}
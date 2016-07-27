#include "Lighting.hlsli"

#define BRDF_FROSTBITE 

struct Input {
	float4 mPosH : SV_POSITION;
	float3 mPosV : POS_VIEW;
	float3 mNormalV : NORMAL_VIEW;
};

struct FrameConstants {
	float4x4 mV;
};
ConstantBuffer<FrameConstants> gFrameConstants : register(b0);

float4 main(const in Input input) : SV_TARGET{
	PunctualLight light;	
	light.mPosV = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), gFrameConstants.mV).xyz;
	light.mRange = 800.0f;
	light.mColor = float3(1.0f, 1.0f, 1.0f);
	light.mPower = 1000.0f;
	
	const float3 luminance = computeLuminance(light, input.mPosV);

	const float3 baseColor = float3(1.0f, 0.71f, 0.29f);
	const float metalMask = 1.0f;
	const float smoothness = 0.6f;
	const float3 reflectance = float3(0.1f, 0.1f, 0.1f);
	const float3 lightDirV = light.mPosV - input.mPosV;
	const float3 normalV = normalize(input.mNormalV);

	float3 illuminance;
#ifdef BRDF_FROSTBITE
	illuminance = brdf_FrostBite(normalV, normalize(-input.mPosV), normalize(lightDirV), baseColor, smoothness, reflectance, metalMask);
#else
	illuminance = brdf_CookTorrance(normalV, normalize(-input.mPosV), normalize(lightDirV), baseColor, smoothness, reflectance, metalMask);
#endif
	return float4(luminance * illuminance, 1.0f);
}
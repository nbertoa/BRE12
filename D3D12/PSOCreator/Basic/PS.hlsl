#include "Lighting.hlsli"

struct Input {
	float4 mPosH : SV_POSITION;
	float3 mPosV : POS_VIEW;
	float3 mNormalV : NORMAL_VIEW;
};

struct FrameConstants {
	float4x4 mV;
};
ConstantBuffer<FrameConstants> gFrameConstants : register(b1);

float smoothDistanceAtt(const float squaredDistance, const float invSqrAttRadius) {
	const float factor = squaredDistance * invSqrAttRadius;
	const float smoothFactor = saturate(1.0f - factor * factor);
	return smoothFactor * smoothFactor;
}

float getDistanceAtt(float3 unormalizedLightVector, float invSqrAttRadius) {
	const float sqrDist = dot(unormalizedLightVector, unormalizedLightVector);
	float attenuation = 1.0f / (max(sqrDist, 0.01f * 0.01f));
	attenuation *= smoothDistanceAtt(sqrDist, invSqrAttRadius);
	return attenuation;
}

float getAngleAtt(const float3 normalizedLightVector, const float3 lightDir, const float lightAngleScale, const float lightAngleOffset) {
	// On the CPU
	// float lightAngleScale = 1.0 f / max (0.001f, ( cosInner - cosOuter ));
	// float lightAngleOffset = -cosOuter * angleScale ;
	const float cd = dot(lightDir, normalizedLightVector);
	float attenuation = saturate(cd * lightAngleScale + lightAngleOffset);
	// smooth the transition
	attenuation *= attenuation;
	return attenuation;
}

float4 main(const in Input input) : SV_TARGET{
	const float3 normalV = normalize(input.mNormalV);
	const float3 lightPosV = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), gFrameConstants.mV).xyz;
	const float lightRadius = 80.0f;
	float3 lightColor = float3(1.0f, 0.0f, 0.0f);
	const float lightPower = 1.0f;
	const float3 lightDir = lightPosV - input.mPosV;

	// Process punctual light
	const float3 unnormalizedLightVector = lightPosV - input.mPosV;
	const float lightInvSqrAttRadius = 1.0f / (lightRadius * lightRadius);
	const float att = getDistanceAtt(unnormalizedLightVector, lightInvSqrAttRadius);
	lightColor = lightPower * lightColor / (4.0f * 3.141592f);

	MaterialData data;
	data.BaseColor = float3(0.0f, 1.0f, 1.0f);
	data.Smoothness = 1.0f;
	data.MetalMask = 0.0f;
	data.Curvature = 1.0f;
	const float3 final = att * lightColor * brdf(normalV, normalize(-input.mPosV), normalize(lightDir), data);
	return float4(final, 1.0f);
}
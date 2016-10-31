#ifndef LIGHTS_H
#define LIGHTS_H

//
// Constants
//
#define PI 3.141592f

//
// Punctual light
//

struct PunctualLight {
	float4 mLightPosVAndRange;
	float4 mLightColorAndPower;
};

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
	const float cd = dot(lightDir, normalizedLightVector);
	float attenuation = saturate(cd * lightAngleScale + lightAngleOffset);

	// Smooth the transition
	attenuation *= attenuation;

	return attenuation;
}

float3 computePunctualLightDirectLightContribution(PunctualLight light, const float3 geomPosV, const float3 normalV, const float cutoff) {
	// Calculate normalized light vector and distance to sphere light surface
	const float r = light.mLightPosVAndRange.w;
	float3 L = light.mLightPosVAndRange.xyz - geomPosV;
	const float distance = length(L);
	const float d = max(distance - r, 0);
	L /= distance;

	// calculate basic attenuation
	const float denom = d / r + 1;
	float att = 1 / (denom * denom);

	// scale and bias attenuation such that:
	//   attenuation == 0 at extent of max influence
	//   attenuation == 1 when d == 0
	att = (att - cutoff) / (1 - cutoff);
	att = max(att, 0);

	return att * light.mLightColorAndPower.w * light.mLightColorAndPower.xyz * saturate(dot(L, normalV));
}

float3 computePunctualLightFrostbiteLightContribution(PunctualLight light, const float3 geomPosV, const float3 normalV) {
	const float3 lightV = light.mLightPosVAndRange.xyz - geomPosV;
	const float range = light.mLightPosVAndRange.w;
	const float lightInvSqrAttRadius = 1.0f / (range * range);
	const float att = getDistanceAtt(lightV, lightInvSqrAttRadius);
	return att * light.mLightColorAndPower.w * light.mLightColorAndPower.xyz * saturate(dot(normalize(lightV), normalV)) / (4.0f * PI);
}

#endif
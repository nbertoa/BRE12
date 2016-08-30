#ifndef LIGHTING_HEADER
#define LIGHTING_HEADER

//
// Constants
//
#define PI 3.141592f

//
// sRGB <-> Linear 
// 
float3 approximationSRgbToLinear(in float3 sRGBCol) {
	return pow(sRGBCol, 2.2);
}

float3 accurateSRGBToLinear(in float3 sRGBCol) {
	float3 linearRGBLo = sRGBCol / 12.92;
	float3 linearRGBHi = pow((sRGBCol + 0.055) / 1.055, 2.4);
	float3 linearRGB = (sRGBCol <= 0.04045) ? linearRGBLo : linearRGBHi;
	return linearRGB;
}

float3 approximationLinearToSRGB(in float3 linearCol) {
	return pow(linearCol, 1 / 2.2);
}

float3 accurateLinearToSRGB(in float3 linearCol) {
	const float3 sRGBLo = linearCol * 12.92;
	const float3 sRGBHi = (pow(abs(linearCol), 1.0 / 2.4) * 1.055) - 0.055;
	const float3 sRGB = (linearCol <= 0.0031308) ? sRGBLo : sRGBHi;
	return sRGB;
}

//
// Lighting
//

// The geometry factor gives the chance that a microfacet with a given orientation
// (again, the half - angle direction is the relevant one) is
// lit and visible (in other words, not shadowed and / or masked) from the given light and view directions.
float V_SmithGGXCorrelated(float NdotL, float NdotV, float alphaG) {
	// Original formulation of G_SmithGGX Correlated
	// lambda_v = (-1 + sqrt ( alphaG2 * (1 - NdotL2 ) / NdotL2 + 1)) * 0.5 f;
	// lambda_l = (-1 + sqrt ( alphaG2 * (1 - NdotV2 ) / NdotV2 + 1)) * 0.5 f;
	// G_SmithGGXCorrelated = 1 / (1 + lambda_v + lambda_l );
	// V_SmithGGXCorrelated = G_SmithGGXCorrelated / (4.0 f * NdotL * NdotV );

	// This is the optimize version
	float alphaG2 = alphaG * alphaG;
	// Caution : the " NdotL *" and " NdotV *" are explicitely inversed , this is not a mistake .
	float Lambda_GGXV = NdotL * sqrt((-NdotV * alphaG2 + NdotV) * NdotV + alphaG2);
	float Lambda_GGXL = NdotV * sqrt((-NdotL * alphaG2 + NdotL) * NdotL + alphaG2);

	return 0.5f / (Lambda_GGXV + Lambda_GGXL);
}

// The Fresnel reflectance is the fraction of incoming light that is reflected (as opposed to refracted) 
// from an optically flat surface of a given substance.
// It depends on thelight direction and the surface(in this case microfacet) normal.
// This tells us how much of the light hitting the relevant microfacets(the ones facing in the half - angle direction) is reflected.
// 
// Since the reflectance over most angles is close to that at normal incidence, 
// the normal-incidence reflectance - F() at 0 degrees - is the surface’s characteristic specular color.
//
// Metals have relatively bright specular colors. Note that metals have no subsurface term, 
// so the surface Fresnel reflectance is the material’s only source of color.
//
// Note that for non-metals the specular colors are achromatic (gray) and are relatively dark (especially if excluding gems and crystals).
// Most non-metals also have a subsurface(or diffuse) color in addition to their Fresnel(or specular) reflectance.
//
// Schlick fresnel calculation
// f0 is the normal incidence reflectance (F() at 0 degrees, used as specular color)
// f90 is the reflectance at 90 degrees
float3 F_Schlick(const float3 f0, const float f90, in float dotLH) {
	return f0 + (f90 - f0) * pow(1.0f - dotLH, 5.0f);
}

float Fd_Disney(const float dotVN, const float dotLN, const float dotLH, float linearRoughness) {
	float energyBias = lerp(0.0f, 0.5f, linearRoughness);
	float energyFactor = lerp(1.0, 1.0 / 1.51, linearRoughness);
	float fd90 = energyBias + 2.0 * dotLH * dotLH * linearRoughness;
	float f0 = 1.0f;
	float lightScatter = F_Schlick(f0, fd90, dotLN).x;
	float viewScatter = F_Schlick(f0, fd90, dotVN).x;
	return lightScatter * viewScatter * energyFactor;
}

float D_GGX_TR(const float dotNH, const float alpha) {
	// Divide by PI is applied later
	const float alphaSqr = alpha * alpha;
	const float f = (dotNH * alphaSqr - dotNH) * dotNH + 1.0f; 
	return alphaSqr / (f * f);
}

float3 brdf_FrostBite(const float3 N, const float3 V, const float3 L, const float3 baseColor, const float smoothness, const float reflectance, const float metalMask) {
	const float roughness = 1.0f - smoothness;
	const float linearRoughness = roughness * roughness;

	const float3 H = normalize(V + L);
	const float dotNL = saturate(dot(N, L));
	const float dotNV = abs(dot(N, V)) + 1e-5f; // avoid artifact
	const float dotNH = saturate(dot(N, H));
	const float dotLH = saturate(dot(L, H));

	// Specular BRDF
	const float3 f0 = (1.0f - metalMask) * float3(reflectance, reflectance, reflectance) + baseColor * metalMask;
	const float f90 = saturate(50.0f * dot(f0, 0.33f));
	const float3 F = F_Schlick(f0, 1.0f, dotLH);
	const float Vis = V_SmithGGXCorrelated(dotNV, dotNL, roughness);
	const float D = D_GGX_TR(dotNH, roughness);
	const float3 Fr = F * Vis * D / PI;

	// Diffuse BRDF
	const float Fd = Fd_Disney(dotNV, dotNL, dotLH, linearRoughness) / PI;
	const float3 diffuseColor = (1.0f - metalMask) * baseColor;

	return dotNL * (Fr + diffuseColor * Fd) / PI;
}

float GlV(const float dotNV, const float k) {
	return 1.0f / (dotNV * (1.0f - k) + k);
}

float3 brdf_CookTorrance(const float3 N, const float3 V, const float3 L, const float3 baseColor, const float smoothness, const float reflectance, const float metalMask) {	
	const float roughness = 1.0f - smoothness;
	const float alpha = roughness * roughness;

	const float3 H = normalize(V + L);
	const float dotNL = saturate(dot(N, L));
	const float dotNV = saturate(dot(N, V));
	const float dotNH = saturate(dot(N, H));
	const float dotLH = saturate(dot(L, H));

	// D GGX
	const float alphaSqr = alpha * alpha;
	const float denom = dotNH * dotNH * (alphaSqr - 1.0f) + 1.0f;
	const float D = alphaSqr / (PI * denom * denom);

	// F Schlick
	const float3 f0 = (1.0f - metalMask) * float3(reflectance, reflectance, reflectance) + baseColor * metalMask;
	const float dotLH5 = pow(1.0f - dotLH, 5.0f);
	const float3 F = f0 + (1.0f - f0) * (dotLH5);

	// V Schlick approximation of Smith solved with GGX
	const float k = alpha / 2.0f;
	const float3 vis = GlV(dotNL, k) * GlV(dotNV, k);

	const float3 specular = dotNL * D * F * vis;
	const float3 diffuse = (1.0f - metalMask) * baseColor;
	return dotNV * (specular + diffuse);
}

//
// Attenuation
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
	// On the CPU
	// float lightAngleScale = 1.0 f / max (0.001f, ( cosInner - cosOuter ));
	// float lightAngleOffset = -cosOuter * angleScale ;
	const float cd = dot(lightDir, normalizedLightVector);
	float attenuation = saturate(cd * lightAngleScale + lightAngleOffset);
	// smooth the transition
	attenuation *= attenuation;
	return attenuation;
}

float3 computePunctualLightDirectLuminance(PunctualLight light, const float3 geomPosV, const float3 normalV, const float cutoff) {
	// calculate normalized light vector and distance to sphere light surface
	float r = light.mLightPosVAndRange.w;
	float3 L = light.mLightPosVAndRange.xyz - geomPosV;
	float distance = length(L);
	float d = max(distance - r, 0);
	L /= distance;

	// calculate basic attenuation
	float denom = d / r + 1;
	float attenuation = 1 / (denom * denom);

	// scale and bias attenuation such that:
	//   attenuation == 0 at extent of max influence
	//   attenuation == 1 when d == 0
	attenuation = (attenuation - cutoff) / (1 - cutoff);
	attenuation = max(attenuation, 0);

	return light.mLightColorAndPower.xyz * saturate(dot(L, normalV)) * attenuation;
}

float3 computePunctualLightFrostbiteLuminance(PunctualLight light, const float3 geomPosV, const float3 normalV) {
	const float3 lightV = light.mLightPosVAndRange.xyz - geomPosV;
	const float range = light.mLightPosVAndRange.w;
	const float lightInvSqrAttRadius = 1.0f / (range * range);
	const float att = getDistanceAtt(lightV, lightInvSqrAttRadius);
	return att * light.mLightColorAndPower.w * light.mLightColorAndPower.xyz * saturate(dot(normalize(lightV), normalV)) / (4.0f * PI);
}

#endif
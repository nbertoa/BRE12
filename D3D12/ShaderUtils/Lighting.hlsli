#ifndef LIGHTING_HEADER
#define LIGHTING_HEADER

//
// Constants
//
#define PI 3.141592f
#define F0_NON_METALS 0.04f

//
// Lighting
//

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

float G_SmithGGX(const float dotNL, const float dotNV, float alpha) {
	const float alphaSqr = alpha * alpha;
	const float G_V = dotNV + sqrt((dotNV - dotNV * alphaSqr) * dotNV + alphaSqr);
	const float G_L = dotNL + sqrt((dotNL - dotNL * alphaSqr) * dotNL + alphaSqr);

	return rcp(G_V * G_L);
}

// f0 is the normal incidence reflectance (F() at 0 degrees, used as specular color)
// f90 is the reflectance at 90 degrees
float3 F_Schlick(const float3 f0, const float f90, const float dotLH) {
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

float3 Fd_Lambert(const float3 diffuseColor) {
	return diffuseColor * rcp(PI);
}

//
// Normal distribution functions 
//
// Typically, m is called roughness that can be remapped as (1 - smoothness) ^ N

float D_GGX_TR(const float m, const float dotNH) {
	const float m2 = m * m;
	const float f = (dotNH * m2 - dotNH) * dotNH + 1.0f; 
	return m2 / (f * f * PI);
}

float D_TR(const float m, const float dotNH) {
	const float m2 = m * m;
	const float denom = dotNH * dotNH * (m2 - 1.0f) + 1.0f;
	return m2 / (PI * denom * denom);
}

#define FD_DISNEY
#define V_SMITH

float3 brdf(const float3 N, const float3 V, const float3 L, const float3 baseColor, const float smoothness, const float metalMask, const float3 reflectionColor) {
	const float roughness = 1.0f - smoothness;
	const float alpha = roughness * roughness;

	const float3 H = normalize(V + L);
	const float dotNL = saturate(dot(N, L));
	const float dotNV = saturate(dot(N, V));
	const float dotNH = saturate(dot(N, H));
	const float dotLH = saturate(dot(L, H));

	// Specular term
	const float D = D_TR(alpha, dotNH);

	const float3 f0 = (1.0f - metalMask) * float3(F0_NON_METALS, F0_NON_METALS, F0_NON_METALS) + baseColor * metalMask;
	const float3 F = F_Schlick(f0, 1.0f, dotLH);
		
#ifdef V_SMITH
	const float G = V_SmithGGXCorrelated(dotNV, dotNL, alpha);
#else
	const float G = G_SmithGGX(dotNL, dotNV, alpha);
#endif

	const float3 Fs = D * F * G * reflectionColor;

	// Diffuse term
	const float3 diffuseColor = (1.0f - metalMask) * baseColor;
	float3 Fd = Fd_Lambert(diffuseColor);
#ifdef FD_DISNEY
	Fd *= Fd_Disney(dotNV, dotNL, dotLH, roughness);
#endif

	return Fd + Fs;
}

#endif
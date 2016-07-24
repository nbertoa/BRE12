#ifndef LIGHTING_HEADER
#define LIGHTING_HEADER

//
// Constants
//
#define PI 3.14159f

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

struct DirectionalLight {
	float3 Color;
	float3 Direction;
};

struct PointLight {
	float3 LuminousPower;
	float3 Color;
	float3 Pos;
	float Range;
};

struct MaterialData {
	float3 BaseColor;
	float Smoothness;
	float Curvature;
	float MetalMask;
};

float G_SmithGGXCorrelated(float NdotL, float NdotV, float alpha) {
	// Original formulation of G_SmithGGX Correlated
	// lambda_v = (-1 + sqrt ( alphaG2 * (1 - NdotL2 ) / NdotL2 + 1)) * 0.5 f;
	// lambda_l = (-1 + sqrt ( alphaG2 * (1 - NdotV2 ) / NdotV2 + 1)) * 0.5 f;
	// G_SmithGGXCorrelated = 1 / (1 + lambda_v + lambda_l );
	// V_SmithGGXCorrelated = G_SmithGGXCorrelated / (4.0 f * NdotL * NdotV );

	// This is the optimize version
	float alphaG2 = alpha * alpha;
	// Caution : the " NdotL *" and " NdotV *" are explicitely inversed , this is not a mistake .
	float Lambda_GGXV = NdotL * sqrt((-NdotV * alphaG2 + NdotV) * NdotV + alphaG2);
	float Lambda_GGXL = NdotV * sqrt((-NdotL * alphaG2 + NdotL) * NdotL + alphaG2);

	return 0.5f / (Lambda_GGXV + Lambda_GGXL);
}

float3 F_Schlick(const float3 f0, const float f90, in float dotLH) {
	return f0 + (f90 - f0) * pow(1.0f - dotLH, 5.0f);
}

float Fd_Disney(const float dotVN, const float dotLN, const float dotLH, float linearRoughness) {
	float energyBias = lerp(0, 0.5, linearRoughness);
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

float3 brdf(const float3 N, const float3 V, const float3 L, const MaterialData data) {
	const float roughness = 1.0f - data.Smoothness;
	const float linearRoughness = roughness * roughness;

	const float dotNV = abs(dot(N, V)) + 1e-5f; // avoid artifact
	const float3 H = normalize(V + L);
	const float dotLH = saturate(dot(L, H));
	const float dotNH = saturate(dot(N, H));
	const float dotNL = saturate(dot(N, L));

	// Specular BRDF
	const float3 f0 = (1.0f - data.MetalMask) * float3(0.04f, 0.04f, 0.04f) + data.BaseColor * data.MetalMask;
	const float f90 = saturate(50.0 * dot(f0, 0.33));
	const float3 F = F_Schlick(f0, 1.0f, dotLH);
	const float G = G_SmithGGXCorrelated(dotNV, dotNL, linearRoughness);
	const float D = D_GGX_TR(dotNH, linearRoughness);
	const float3 Fr = F * G * D;
	const float specularColor = 0.5f * data.Curvature;

	// Diffuse BRDF
	const float Fd = Fd_Disney(dotNV, dotNL, dotLH, roughness);
	const float3 diffuseColor = (1.0f - data.MetalMask) * data.BaseColor * data.Curvature;

	// Put all the parts together to generate the final color	
	return dotNL * (specularColor * Fr + diffuseColor * Fd) / PI;
}

#endif
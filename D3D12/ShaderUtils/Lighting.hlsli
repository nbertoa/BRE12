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

//
// Specular term:
//

// fSpecular = F(l, h) * G(l, v, h) * D(h) / 4 * dotNL * dotNV
//
// D(h) is the microgeometry normal distribution function(NDF) evaluated at the half-vector h; in other words, the
// concentration (relative to surface area) of surface points which are oriented such that they could reflect
// light from l into v.
//
// G(l, v, h) is the geometry function; it tells us the percentage of surface points with m = h that 
// are not shadowed or masked, as a function of the light direction l and the view direction v.
//
// Therefore, the product of D(h) and G(l; v; h) gives us the concentration of active surface
// points, the surface points that actively participate in the reflectance by successfully 
// reflecting light from l into v.
//
// F(l; h) is the Fresnel reflectance of the active surface points as a function of the light
// direction l and the active microgeometry normal m = h.
//
// It tells us how much of the incoming light is reflected from each of the active surface points.
//
// Finally, the denominator 4 * dotNL * dotNV is a correction factor, 
// which accounts for quantities being transformed between the local space of the microgeometry
// and that of the overall macrosurface.
//

//
// Fresnel Reflectance:
//
// The Fresnel reflectance function computes the fraction of light reflected from an optically
// flat surface.
//
// Its value depends on two things : the incoming angle (the angle between the light vector and the surface
// normal - also referred to as the incident angle or angle of incidence) and the refractive index of the
// material.
//
// Since the refractive index may vary over the visible spectrum, the Fresnel reflectance is a
// spectral quantity - for production purposes, an RGB triple.
//
// We also know that each of the RGB values have to lie within the 0 to 1 range, since a surface cannot 
// reflect less than 0 % or more than 100 % of the incoming light.
//
// Since the Fresnel reflectance stays close to the value for 0 over most of the visible parts of a given
// 3D scene, we can think of this value(which we will denote F0) as the characteristic specular reflectance
// of the material.
// 
// This value has all the properties of what is typically thought of as a "color", it is
// composed of RGB values between 0 and 1, and it is a measure of selective reflectance of light.
// For this reason, we will also refer to this value as the specular color of the surface.
//

// Schlick fresnel:
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

//
// Normal distribution Function:
//
// The microgeometry in most surfaces does not have uniform distributions of surface point orientations.
//
// More surface points have normals pointing "up" (towards the macroscopic surface normal n) than
// "sideways" (away from n). 
//
// The statistical distribution of surface orientations is dened via the microgeometry 
// normal distribution function D(m). 
//
// Unlike F(), the value of D() is not restricted to lie between 0 and 1, although values must be non-negative, 
// they can be arbitrarily large (indicating a very high concentration of surface points with normals 
// pointing in a particular direction).
//
// Also, unlike F(), the function D() is not spectral nor RGB valued, but scalar valued.
//
// In microfacet BRDF terms, D() is evaluated for the direction h, to help determine 
// the concentration of potentially active surface points(those for which m = h).
//
// The function D() determines the size, brightness, and shape of the specular highlight.
//

// GGX/Trowbridge-Reitz
// m is roughness
float D_TR(const float m, const float dotNH) {
	const float m2 = m * m;
	const float denom = dotNH * dotNH * (m2 - 1.0f) + 1.0f;
	return m2 / (PI * denom * denom);
}

//
// Geometry function:
//
// The geometry function G(l, v, h) represents the probability that surface points with a given microgeometry
// normal m will be visible from both the light direction l and the view direction v.
//
// In the microfacet BRDF, m is replaced with h (for similar reasons as in the previous two terms).
//
// Since the function G() represents a probability, its value is a scalar and constrained to lie between 0 and 1.
//
// The geometry function typically does not introduce any new parameters to the BRDF; it either has no parameters, or
// uses the roughness parameter(s) of the D() function.
//
// In many cases, the geometry function partially cancels out the dotNL * dotNV denominator in fSpecular equation, replacing it with some other expression.
// The geometry function is essential for BRDF energy conservation, without such a term the BRDF
// can reflect arbitrarily more light energy than it receives.
//
// A key part of the microfacet BRDF derivation relates to the ratio between the active surface area
// (the area covered by surface regions that reflect light energy from l to v) and 
// the total surface area of the macroscopic surface.If shadowing and masking
// are not accounted for, then the active area may exceed the total area, an obvious impossibility which
// can lead to the BRDF not conserving energy, in some cases by a huge amount

float V_SmithGGXCorrelated(float dotNL, float dotNV, float alphaG) {
	// Original formulation of G_SmithGGX Correlated
	// lambda_v = (-1 + sqrt ( alphaG2 * (1 - dotNL2 ) / dotNL2 + 1)) * 0.5 f;
	// lambda_l = (-1 + sqrt ( alphaG2 * (1 - dotNV2 ) / dotNV2 + 1)) * 0.5 f;
	// G_SmithGGXCorrelated = 1 / (1 + lambda_v + lambda_l );
	// V_SmithGGXCorrelated = G_SmithGGXCorrelated / (4.0 f * dotNL * dotNV );

	// This is the optimized version
	float alphaG2 = alphaG * alphaG;
	// Caution : the " dotNL *" and " dotNV *" are explicitely inversed , this is not a mistake .
	float Lambda_GGXV = dotNL * sqrt((-dotNV * alphaG2 + dotNV) * dotNV + alphaG2);
	float Lambda_GGXL = dotNV * sqrt((-dotNL * alphaG2 + dotNL) * dotNL + alphaG2);

	return 0.5f / (Lambda_GGXV + Lambda_GGXL);
}

float G_SmithGGX(const float dotNL, const float dotNV, float alpha) {
	const float alphaSqr = alpha * alpha;
	const float G_V = dotNV + sqrt((dotNV - dotNV * alphaSqr) * dotNV + alphaSqr);
	const float G_L = dotNL + sqrt((dotNL - dotNL * alphaSqr) * dotNL + alphaSqr);

	return rcp(G_V * G_L);
}

//
// Diffuse term:
//

// Lambertian diffuse term
float3 Fd_Lambert(const float3 diffuseColor) {
	return diffuseColor / PI;
}

float Fr_DisneyDiffuse(const float dotNV, const float dotNL, const float dotLH, const float linearRoughness)
{
	const float energyBias = lerp(0, 0.5, linearRoughness);
	const float energyFactor = lerp(1.0, 1.0 / 1.51, linearRoughness);
	const float fd90 = energyBias + 2.0 * dotLH * dotLH * linearRoughness;
	const float3 f0 = float3 (1.0f, 1.0f, 1.0f);
	const float lightScatter = F_Schlick(f0, fd90, dotNL).r;
	const float viewScatter = F_Schlick(f0, fd90, dotNV).r;

	return lightScatter * viewScatter * energyFactor;
}

//
// BRDF
//
#define V_SMITH

float3 DiffuseBrdf(const float3 baseColor, const float metalMask) {
	const float3 diffuseColor = (1.0f - metalMask) * baseColor;
	return Fd_Lambert(diffuseColor);
}

float3 SpecularBrdf(const float3 N, const float3 V, const float3 L, const float3 baseColor, const float smoothness, const float metalMask) {
	const float roughness = 1.0f - smoothness;

	// Disney's reparametrization of roughness
	const float alpha = roughness * roughness;

	const float3 H = normalize(V + L);
	const float dotNL = abs(dot(N, V)) + 1e-5f;
	const float dotNV = abs(dot(N, V)) + 1e-5f; // avoid artifacts
	const float dotNH = saturate(dot(N, H));
	const float dotLH = saturate(dot(L, H));

	//
	// Specular term: (D * F * G) / (4 * dotNL * dotNV)
	//
	
	const float D = D_TR(roughness, dotNH);
		
	const float3 f0 = (1.0f - metalMask) * float3(F0_NON_METALS, F0_NON_METALS, F0_NON_METALS) + baseColor * metalMask;
	const float3 F = F_Schlick(f0, 1.0f, dotLH);
		
	// G / (4 * dotNL * dotNV)
#ifdef V_SMITH
	const float G_Correlated = V_SmithGGXCorrelated(dotNV, dotNL, alpha);
#else
	const float G_Correlated = G_SmithGGX(dotNL, dotNV, alpha);
#endif

	return D * F * G_Correlated;
}

#endif
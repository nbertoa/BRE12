#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Lighting.hlsli>
#include <ShaderUtils/Lights.hlsli>
#include <ShaderUtils/Utils.hlsli>

struct Input {
	float4 mPosH : SV_POSITION;
	float3 mViewRayV : VIEW_RAY;
	nointerpolation PunctualLight mPunctualLight : PUNCTUAL_LIGHT;
};

Texture2D<float4> NormalV_Smoothness_DepthV : register (t0);
Texture2D<float4> BaseColor_MetalMask : register (t1);

struct Output {
	float4 mColor : SV_Target0;
};

Output main(const in Input input) {
	Output output = (Output)0;

	const int3 screenCoord = int3(input.mPosH.xy, 0);
	
	// Reconstruct geometry position in view space.
	// position = viewRay * depth (view space)
	const float4 normalV_Smoothness_DepthV = NormalV_Smoothness_DepthV.Load(screenCoord);
	const float normalizedDepth = normalV_Smoothness_DepthV.w;
	const float3 viewRay = normalize(input.mViewRayV);
	const float3 geomPosV = viewRay * normalizedDepth;

	PunctualLight light = input.mPunctualLight;

	// Get normal
	const float2 normal = normalV_Smoothness_DepthV.xy;
	const float3 normalV = normalize(Decode(normal));

	const float4 baseColor_metalmask = BaseColor_MetalMask.Load(screenCoord);
	const float smoothness = normalV_Smoothness_DepthV.z;
	const float3 lightDirV = normalize(light.mLightPosVAndRange.xyz - geomPosV);

	// As we are working at view space, we do not need camera position to 
	// compute vector from geometry position to camera.
	const float3 viewV = normalize(-geomPosV);

	const float3 lightContrib = computePunctualLightFrostbiteLightContribution(light, geomPosV, normalV);
			
	const float3 fDiffuse = DiffuseBrdf(baseColor_metalmask.xyz, baseColor_metalmask.w);
	const float3 fSpecular = SpecularBrdf(normalV, viewV, lightDirV, baseColor_metalmask.xyz, smoothness, baseColor_metalmask.w);
		
	const float3 color = lightContrib * (fDiffuse + fSpecular);

	output.mColor = float4(color, 1.0f);

	return output;
}
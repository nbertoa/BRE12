#include "../ShaderUtils/CBuffers.hlsli"
#include "../ShaderUtils/Lighting.hlsli"
#include "../ShaderUtils/Lights.hlsli"
#include "../ShaderUtils/Utils.hlsli"

struct Input {
	float4 mPosH : SV_POSITION;
	float3 mPosV : VIEW_RAY;
	nointerpolation PunctualLight mPunctualLight : PUNCTUAL_LIGHT;
};

ConstantBuffer<ImmutableCBuffer> gImmutableCBuffer : register(b0);

Texture2D<float4> NormalV_Smoothness_Depth : register (t0);
Texture2D<float4> BaseColor_MetalMask : register (t1);

struct Output {
	float4 mColor : SV_Target0;
};

Output main(const in Input input) {
	Output output = (Output)0;

	const int3 screenCoord = int3(input.mPosH.xy, 0);
	
	// Reconstruct geometry position in view space.
	// If sampled normalized depth is 1, then the geometry is at far plane
	// or current pixel belong to a pixel that was not covered by any geometry.
	// If that is the case, we discard it.
	const float4 normalV_Smoothness_Depth = NormalV_Smoothness_Depth.Load(screenCoord);
	const float normalizedDepth = normalV_Smoothness_Depth.w;
	clip(any(normalizedDepth - 1.0f) ? 1 : -1);
	const float farZ = gImmutableCBuffer.mNearZ_FarZ_ScreenW_ScreenH.y;
	const float3 viewRay = float3(input.mPosV.xy * (farZ / input.mPosV.z), farZ);
	const float3 geomPosV = viewRay * normalizedDepth;

	PunctualLight light = input.mPunctualLight;

	const float2 normal = normalV_Smoothness_Depth.xy;
	const float3 normalV = normalize(Decode(normal));

	const float4 baseColor_metalmask = BaseColor_MetalMask.Load(screenCoord);
	const float smoothness = normalV_Smoothness_Depth.z;
	const float3 lightDirV = normalize(light.mLightPosVAndRange.xyz - geomPosV);
	// As we are working at view space, we do not need camera position to 
	// compute vector from geometry position to camera.
	const float3 viewV = normalize(-geomPosV);

	const float3 lightContrib = computePunctualLightFrostbiteLightContribution(light, geomPosV, normalV);

	// Discard if light does not contribute any light.
	clip(any(dot(lightContrib, 1.0f)) ? 1 : -1);

	const float3 fLV = brdf(normalV, viewV, lightDirV, baseColor_metalmask.xyz, smoothness, baseColor_metalmask.w);

	// Discard if brdf does not contribute any light.
	clip(dot(fLV, 1.0f) ? 1 : -1);

	output.mColor = float4(lightContrib * fLV, 1.0f);

	return output;
}
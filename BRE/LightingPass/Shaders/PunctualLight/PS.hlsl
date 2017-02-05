#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Lighting.hlsli>
#include <ShaderUtils/Lights.hlsli>
#include <ShaderUtils/Utils.hlsli>

#include "RS.hlsl"

struct Input {
	float4 mPositionClipSpace : SV_POSITION;
	float3 mCameraToFragmentViewSpace : VIEW_RAY;
	nointerpolation PunctualLight mPunctualLight : PUNCTUAL_LIGHT;
};

ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b0);

Texture2D<float4> Normal_SmoothnessTexture : register (t0);
Texture2D<float4> BaseColor_MetalMaskTexture : register (t1);
Texture2D<float> DepthTexture : register (t2);

struct Output {
	float4 mColor : SV_Target0;
};

[RootSignature(RS)]
Output main(const in Input input) {
	Output output = (Output)0;

	const int3 fragmentScreenSpace = int3(input.mPositionClipSpace.xy, 0);
	
	const float4 normal_smoothness = Normal_SmoothnessTexture.Load(fragmentScreenSpace);
	
	// Compute fragment position in view space
	const float fragmentZNDC = DepthTexture.Load(fragmentScreenSpace);
	const float3 rayViewSpace = normalize(input.mCameraToFragmentViewSpace);
	const float3 fragmentPositionViewSpace = ViewRayToViewPosition(rayViewSpace, fragmentZNDC, gFrameCBuffer.mProjectionMatrix);

	PunctualLight light = input.mPunctualLight;

	// Get normal
	const float2 normal = normal_smoothness.xy;
	const float3 normalViewSpace = normalize(Decode(normal));

	const float4 baseColor_metalmask = BaseColor_MetalMaskTexture.Load(fragmentScreenSpace);
	const float smoothness = normal_smoothness.z;
	const float3 lightDirectionViewSpace = normalize(light.mLightPosVAndRange.xyz - fragmentPositionViewSpace);

	// As we are working at view space, we do not need camera position to 
	// compute vector from geometry position to camera.
	const float3 viewV = normalize(-fragmentPositionViewSpace);

	const float3 lightContribution = computePunctualLightFrostbiteLightContribution(light, fragmentPositionViewSpace, normalViewSpace);
			
	const float3 fDiffuse = DiffuseBrdf(baseColor_metalmask.xyz, baseColor_metalmask.w);
	const float3 fSpecular = SpecularBrdf(normalViewSpace, viewV, lightDirectionViewSpace, baseColor_metalmask.xyz, smoothness, baseColor_metalmask.w);
		
	const float3 color = lightContribution * (fDiffuse + fSpecular);

	output.mColor = float4(color, 1.0f);

	return output;
}
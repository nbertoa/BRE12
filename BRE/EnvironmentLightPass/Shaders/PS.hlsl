#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Lighting.hlsli>
#include <ShaderUtils/Utils.hlsli>

#include "RS.hlsl"

//#define SKIP_ENVIRONMENT_LIGHT

struct Input {
	float4 mPositionNDC : SV_POSITION;
	float3 mCameraToFragmentViewSpace : VIEW_RAY;
};

ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b0);

SamplerState TextureSampler : register (s0);

Texture2D<float4> Normal_SmoothnessTexture : register (t0);
Texture2D<float4> BaseColor_MetalMaskTexture : register (t1);
Texture2D<float> DepthTexture : register (t2);
TextureCube DiffuseCubeMapTexture : register(t3);
TextureCube SpecularCubeMapTexture : register(t4);

struct Output {
	float4 mColor : SV_Target0;
};

[RootSignature(RS)]
Output main(const in Input input){
	Output output = (Output)0;

#ifdef SKIP_ENVIRONMENT_LIGHT
	output.mColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
#else
	const int3 fragmentScreenSpace = int3(input.mPositionNDC.xy, 0);

	const float4 normal_smoothness = Normal_SmoothnessTexture.Load(fragmentScreenSpace);

	// Compute fragment position in view space
	const float fragmentZNDC = DepthTexture.Load(fragmentScreenSpace);
	const float3 rayViewSpace = normalize(input.mCameraToFragmentViewSpace);
	const float3 fragmentPositionViewSpace = ViewRayToViewPosition(rayViewSpace, fragmentZNDC, gFrameCBuffer.mProjectionMatrix);

	const float3 fragmentPositionWorldSpace = mul(float4(fragmentPositionViewSpace, 1.0f), gFrameCBuffer.mInverseViewMatrix).xyz;
	
	const float2 encodedNormal = normal_smoothness.xy;
	const float3 normalViewSpace = normalize(Decode(encodedNormal));
	const float3 normalWorldSpace = normalize(mul(float4(normalViewSpace, 0.0f), gFrameCBuffer.mInverseInverseTransposeViewMatrix).xyz);

	const float4 baseColor_metalmask = BaseColor_MetalMaskTexture.Load(fragmentScreenSpace);
	const float3 baseColor = baseColor_metalmask.xyz;
	const float metalMask = baseColor_metalmask.w;

	// As we are working at view space, we do not need camera position to 
	// compute vector from geometry position to camera.
	const float3 fragmentPositionToCameraViewSpace = normalize(-fragmentPositionViewSpace);

	// Diffuse reflection color.
	// When we sample a cube map, we need to use data in world space, not view space.
	const float3 diffuseReflection = DiffuseCubeMapTexture.Sample(TextureSampler, normalWorldSpace).rgb;
	const float3 diffuseColor = (1.0f - metalMask) * baseColor;
	const float3 indirectFDiffuse = diffuseColor * diffuseReflection;

	// Compute incident vector. 
	// When we sample a cube map, we need to use data in world space, not view space.
	const float3 incidentVectorWorldSpace = fragmentPositionWorldSpace - gFrameCBuffer.mEyePositionWorldSpace.xyz;
	const float3 reflectionVectorWorldSpace = reflect(incidentVectorWorldSpace, normalWorldSpace);

	// Our cube map has 10 mip map levels (0 - 9) based on smoothness
	const float smoothness = normal_smoothness.z;
	const uint mipmap = (1.0f - smoothness) * 9.0f;
	const float3 specularReflection = SpecularCubeMapTexture.SampleLevel(TextureSampler, reflectionVectorWorldSpace, mipmap).rgb;

	// Specular reflection color
	const float3 dielectricColor = float3(0.04f, 0.04f, 0.04f);
	const float3 f0 = lerp(dielectricColor, baseColor, metalMask);
	const float3 F = F_Schlick(f0, 1.0f, dot(fragmentPositionToCameraViewSpace, normalViewSpace));
	const float3 indirectFSpecular = F * specularReflection;

	const float3 color = indirectFDiffuse + indirectFSpecular;

	output.mColor = float4(color, 1.0f);
#endif
	
	return output;
}
#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Lighting.hlsli>
#include <ShaderUtils/Utils.hlsli>

#include "RS.hlsl"

struct Input {
	float4 mPosH : SV_POSITION;
	float3 mViewRayV : VIEW_RAY;
};

ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b0);

SamplerState TexSampler : register (s0);

Texture2D<float4> Normal_Smoothness : register (t0);
Texture2D<float4> BaseColor_MetalMask : register (t1);
Texture2D<float> Depth : register (t2);
TextureCube DiffuseCubeMap : register(t3);
TextureCube SpecularCubeMap : register(t4);

struct Output {
	float4 mColor : SV_Target0;
};

[RootSignature(RS)]
Output main(const in Input input){
	Output output = (Output)0;

	const int3 screenCoord = int3(input.mPosH.xy, 0);

	const float4 normal_smoothness = Normal_Smoothness.Load(screenCoord);

	// Compute fragment position in view space
	const float depthNDC = Depth.Load(screenCoord);
	const float3 viewRayV = normalize(input.mViewRayV);
	const float3 fragPosV = ViewRayToViewPosition(viewRayV, depthNDC, gFrameCBuffer.mP);

	const float3 fragPosW = mul(float4(fragPosV, 1.0f), gFrameCBuffer.mInvV).xyz;
	
	const float2 normal = normal_smoothness.xy;
	const float3 normalV = normalize(Decode(normal));
	const float3 normalW = normalize(mul(float4(normalV, 0.0f), gFrameCBuffer.mInvV).xyz);

	const float4 baseColor_metalmask = BaseColor_MetalMask.Load(screenCoord);

	// As we are working at view space, we do not need camera position to 
	// compute vector from geometry position to camera.
	const float3 fragPosToCameraV = normalize(-fragPosV);

	// Diffuse reflection color.
	// When we sample a cube map, we need to use data in world space, not view space.
	const float3 diffuseReflection = DiffuseCubeMap.Sample(TexSampler, normalW).rgb;
	const float3 diffuseColor = (1.0f - baseColor_metalmask.w) * baseColor_metalmask.xyz;
	const float3 indirectFDiffuse = diffuseColor * diffuseReflection;

	// Compute incident vector. 
	// When we sample a cube map, we need to use data in world space, not view space.
	const float3 incidentVecW = fragPosW - gFrameCBuffer.mEyePosW.xyz;
	const float3 reflectionVecW = reflect(incidentVecW, normalW);
	// Our cube map has 10 mip map levels (0 - 9) based on smoothness
	const float smoothness = normal_smoothness.z;
	const uint mipmap = (1.0f - smoothness) * 9.0f;
	const float3 specularReflection = SpecularCubeMap.SampleLevel(TexSampler, reflectionVecW, mipmap).rgb;

	// Specular reflection color
	const float3 f0 = (1.0f - baseColor_metalmask.w) * float3(0.04f, 0.04f, 0.04f) + baseColor_metalmask.xyz * baseColor_metalmask.w;
	const float3 F = F_Schlick(f0, 1.0f, dot(fragPosToCameraV, normalV));
	const float3 indirectFSpecular = F * specularReflection;

	const float3 color = indirectFDiffuse + indirectFSpecular;

	output.mColor = float4(color, 1.0f);
	
	return output;
}
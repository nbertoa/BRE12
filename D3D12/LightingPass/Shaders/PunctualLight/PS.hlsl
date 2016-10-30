#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Lighting.hlsli>
#include <ShaderUtils/Lights.hlsli>
#include <ShaderUtils/Utils.hlsli>

struct Input {
	float4 mPosH : SV_POSITION;
	float3 mViewRayV : VIEW_RAY;
	nointerpolation PunctualLight mPunctualLight : PUNCTUAL_LIGHT;
};

ConstantBuffer<ImmutableCBuffer> gImmutableCBuffer : register(b0);

Texture2D<float4> Normal_Smoothness : register (t0);
Texture2D<float4> BaseColor_MetalMask : register (t1);
Texture2D<float> Depth : register (t2);

struct Output {
	float4 mColor : SV_Target0;
};

Output main(const in Input input) {
	Output output = (Output)0;

	const int3 screenCoord = int3(input.mPosH.xy, 0);
	
	const float4 normal_smoothness = Normal_Smoothness.Load(screenCoord);

	// Clamp the view space position to the plane at Z = 1
	const float3 viewRay = float3(input.mViewRayV.xy / input.mViewRayV.z, 1.0f);

	// Sample the depth and convert to linear view space Z (assume it gets sampled as
	// a floating point value of the range [0,1])
	const float depth = Depth.Load(screenCoord);
	const float linearDepth = gImmutableCBuffer.mProjectionA_ProjectionB.y / (depth - gImmutableCBuffer.mProjectionA_ProjectionB.x);
	const float3 geomPosV = viewRay * linearDepth;

	PunctualLight light = input.mPunctualLight;

	// Get normal
	const float2 normal = normal_smoothness.xy;
	const float3 normalV = normalize(Decode(normal));

	const float4 baseColor_metalmask = BaseColor_MetalMask.Load(screenCoord);
	const float smoothness = normal_smoothness.z;
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
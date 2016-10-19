#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Lighting.hlsli>
#include <ShaderUtils/Utils.hlsli>

struct Input {
	float4 mPosH : SV_POSITION;
	float3 mViewRayV : VIEW_RAY;
};

Texture2D<float4> Normal_Smoothness : register (t0);
Texture2D<float4> BaseColor_MetalMask : register (t1);
Texture2D<float4> DiffuseReflection : register (t2);
Texture2D<float4> SpecularReflection : register (t3);
Texture2D<float> Depth : register (t4);

struct Output {
	float4 mColor : SV_Target0;
};

Output main(const in Input input){
	Output output = (Output)0;

	const int3 screenCoord = int3(input.mPosH.xy, 0);

	// Reconstruct geometry position in view space.
	// position = viewRay * depth (view space)
	const float4 normal_smoothness = Normal_Smoothness.Load(screenCoord);
	const float depth = Depth.Load(screenCoord);
	const float3 viewRay = normalize(input.mViewRayV);
	const float3 geomPosV = viewRay * depth;
	
	// Get normal
	const float2 normal = normal_smoothness.xy;
	const float3 normalV = normalize(Decode(normal));

	const float4 baseColor_metalmask = BaseColor_MetalMask.Load(screenCoord);

	// As we are working at view space, we do not need camera position to 
	// compute vector from geometry position to camera.
	const float3 viewV = normalize(-geomPosV);

	// Specular reflection color
	const float3 reflectionColor = SpecularReflection.Load(screenCoord).xyz;
	const float3 f0 = (1.0f - baseColor_metalmask.w) * float3(0.04f, 0.04f, 0.04f) + baseColor_metalmask.xyz * baseColor_metalmask.w;
	const float3 F = F_Schlick(f0, 1.0f, dot(viewV, normalV));
	const float3 indirectFSpecular = F * reflectionColor;

	const float3 color = indirectFSpecular;

	output.mColor = float4(color, 1.0f);
	
	return output;
}
//#include "../ShaderUtils/Lighting.hlsli"

//#define BRDF_FROSTBITE_ILLUMINANCE 
//#define BRDF_FROSTBITE_LUMINANCE

struct Input {
	float4 mPosH : SV_POSITION;
	float3 mPosV : POS_VIEW;
	float3 mNormalV : NORMAL_VIEW;
};

struct Material {
	float4 mBaseColor_MetalMask;
	float4 mReflectance_Smoothness;
};
ConstantBuffer<Material> gMaterial : register(b0);

struct FrameConstants {
	float4x4 mV;
};
ConstantBuffer<FrameConstants> gFrameConstants : register(b1);

struct Output {	
	float4 mNormalV : SV_Target0;
	float4 mPosV : SV_Target1;
	float4 mBaseColor_MetalMask : SV_Target2;
	float4 mReflectance_Smoothness : SV_Target3;
	//float4 mColor : SV_Target4;
};

Output main(const in Input input) {
	/*PunctualLight light;
	light.mPosV = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), gFrameConstants.mV).xyz;
	light.mRange = 100.0f;
	light.mColor = float3(1.0f, 1.0f, 1.0f);
	light.mPower = 100.0f;	
	const float3 baseColor = gMaterial.mBaseColor_MetalMask.xyz;
	const float metalMask = gMaterial.mBaseColor_MetalMask.w;
	const float smoothness = gMaterial.mReflectance_Smoothness.w;
	const float3 reflectance = gMaterial.mReflectance_Smoothness.xyz;
	const float3 lightDirV = light.mPosV - input.mPosV;
	
	float3 luminance;
#ifdef BRDF_FROSTBITE_LUMINANCE
	luminance = computePunctualLightFrostbiteLuminance(light, input.mPosV);
#else
	luminance = computePunctualLightDirectLuminance(light, input.mPosV, normalize(input.mNormalV), 0.0000f);
#endif

	float3 illuminance;
#ifdef BRDF_FROSTBITE_ILLUMINANCE
	illuminance = brdf_FrostBite(normalize(input.mNormalV), normalize(-input.mPosV), normalize(lightDirV), baseColor, smoothness, reflectance, metalMask);
#else
	illuminance = brdf_CookTorrance(normalize(input.mNormalV), normalize(-input.mPosV), normalize(lightDirV), baseColor, smoothness, reflectance, metalMask);
#endif*/

	Output output = (Output)0;
	output.mNormalV = float4(normalize(input.mNormalV), 0.0f);
	output.mPosV = float4(input.mPosV, 1.0f);
	output.mBaseColor_MetalMask = gMaterial.mBaseColor_MetalMask;
	output.mReflectance_Smoothness = gMaterial.mReflectance_Smoothness;
	//output.mColor = float4(luminance * illuminance, 1.0f);
	
	return output;
}
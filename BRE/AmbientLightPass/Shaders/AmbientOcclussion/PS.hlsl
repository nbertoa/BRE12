#include <ShaderUtils/AmbientOcclusion.hlsli>
#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Utils.hlsli>

#define VERSION1
#define SAMPLE_KERNEL_SIZE 14U
#define NOISE_SCALE float2(1920.0f / 4.0f, 1080.0f / 4.0f)
#define OCCLUSION_RADIUS 2.0f
#define SURFACE_EPSILON 0.05f
#define OCCLUSION_FADE_START 0.2f
#define OCCLUSION_FADE_END 1000.0f

struct Input {
	float4 mPosH : SV_POSITION;
	float3 mViewRayV : VIEW_RAY;
	float2 mTexCoordO : TEXCOORD;
};

ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b0);

SamplerState TexSampler : register (s0);

Texture2D<float4> Normal_Smoothness : register (t0);
Texture2D<float> Depth : register (t1);
StructuredBuffer<float4> SampleKernel : register(t2);
Texture2D<float4> NoiseTexture : register (t3); 

struct Output {
	float mAccessibility : SV_Target0;
};

Output main(const in Input input) {
	Output output = (Output)0;
	return output;

	/*const int3 screenCoord = int3(input.mPosH.xy, 0);

	// Compute fragment position in view space
	const float depthNDC = Depth.Load(screenCoord);
	const float3 fragPosV = ViewRayToViewPosition(input.mViewRayV, depthNDC, gFrameCBuffer.mP);

	// Get normal
	const float2 normal = Normal_Smoothness.Load(screenCoord).xy;
	const float3 normalV = normalize(Decode(normal));

	// Construct a change-of-basis matrix to reorient our sample kernel
	// along the origin's normal.
	const float3 noiseVec = NoiseTexture.SampleLevel(TexSampler, 4.0f * input.mTexCoordO, 0.0f).xyz * 2.0f - 1.0f;
	const float3 tangentV = normalize(noiseVec - normalV * dot(noiseVec, normalV));
	const float3 bitangentV = normalize(cross(normalV, tangentV));
	const float3x3 sampleKernelMatrix = float3x3(tangentV, bitangentV, normalV);
	
	float occlusionSum = 0.0f;
	for (uint i = 0U; i < SAMPLE_KERNEL_SIZE; ++i) {
		float occlusion;
		
#ifdef VERSION1
	occlusion = SSAOVersion1(
		SampleKernel[i].xyz,
		sampleKernelMatrix,
		gFrameCBuffer.mP,
		OCCLUSION_RADIUS,
		fragPosV,
		Depth);

#else
	occlusion = SSAOVersion2(
		SampleKernel[i].xyz,
		noiseVec,
		normalV,
		gFrameCBuffer.mP,
		OCCLUSION_RADIUS,
		fragPosV,
		Depth,
		SURFACE_EPSILON,
		OCCLUSION_FADE_START,
		OCCLUSION_FADE_END);
#endif

		occlusionSum += occlusion;
	}

	output.mAccessibility = 1.0f - (occlusionSum / SAMPLE_KERNEL_SIZE);

	// Sharpen the contrast of the SSAO map to make the SSAO affect more dramatic.
	output.mAccessibility =  saturate(pow(output.mAccessibility, 2.0f));

	return output;*/
}
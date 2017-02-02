#include <ShaderUtils/AmbientOcclusion.hlsli>
#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Utils.hlsli>

#include "RS.hlsl"

#define SAMPLE_KERNEL_SIZE 128U
#define SCREEN_WIDTH 1920.0f
#define SCREEN_HEIGHT 1080.0f
#define NOISE_TEXTURE_DIMENSION 4.0f
#define NOISE_SCALE float2(SCREEN_WIDTH / NOISE_TEXTURE_DIMENSION, SCREEN_HEIGHT/ NOISE_TEXTURE_DIMENSION)
#define OCCLUSION_RADIUS 1.69f
#define SSAO_POWER 2.57f

//#define SKIP_AMBIENT_OCCLUSION

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

[RootSignature(RS)]
Output main(const in Input input) {
	Output output = (Output)0;

#ifdef SKIP_AMBIENT_OCCLUSION
	output.mAccessibility = 1.0f;
#else
	const int3 screenCoord = int3(input.mPosH.xy, 0);

	const float depthNDC = Depth.Load(screenCoord);
	const float3 viewRayV = normalize(input.mViewRayV);
	const float4 fragPosV = float4(ViewRayToViewPosition(viewRayV, depthNDC, gFrameCBuffer.mP), 1.0f);

	const float2 normal = Normal_Smoothness.Load(screenCoord).xy;
	const float3 normalV = normalize(Decode(normal));

	// Construct a change-of-basis matrix to reorient our sample kernel
	// along the origin's normal.
	const float3 noiseVec = NoiseTexture.SampleLevel(TexSampler, NOISE_SCALE * input.mTexCoordO, 0.0f).xyz * 2.0f - 1.0f;
	const float3 tangentV = normalize(noiseVec - normalV * dot(noiseVec, normalV));
	const float3 bitangentV = normalize(cross(normalV, tangentV));
	const float3x3 sampleKernelMatrix = float3x3(tangentV, bitangentV, normalV);

	// Iterate over sample kernel
	float occlusionSum = 0.0f;
	for (uint i = 0U; i < SAMPLE_KERNEL_SIZE; ++i) {
		// Convert sample to view space
		const float4 offset = float4(mul(SampleKernel[i].xyz, sampleKernelMatrix), 0.0f);

		// Get position resulting from the displacement of fragPosV by offsetV
		const float4 samplePosV = fragPosV + offset * OCCLUSION_RADIUS;

		// Convert sample position to NDC and sample depth at that position in depth buffer.
		float4 samplePosH = mul(samplePosV, gFrameCBuffer.mP);
		samplePosH.xy /= samplePosH.w;
	
		const int2 sampleViewportSpace = NdcToViewportCoordinates(samplePosH.xy, 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT);
		const float sampleDepthNDC = Depth.Load(int3(sampleViewportSpace, 0));

		// Convert sample depth from NDC to view space
		const float sampleDepthV = NdcDepthToViewDepth(sampleDepthNDC, gFrameCBuffer.mP);

		occlusionSum += (sampleDepthV <= samplePosV.z ? 1.0 : 0.0);
	}

	output.mAccessibility = 1.0f - (occlusionSum / SAMPLE_KERNEL_SIZE);
#endif

	// Sharpen the contrast of the SSAO map to make the SSAO affect more dramatic.
	output.mAccessibility = saturate(pow(output.mAccessibility, SSAO_POWER));

	return output;
}
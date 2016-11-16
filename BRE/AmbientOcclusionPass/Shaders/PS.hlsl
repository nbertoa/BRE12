#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Utils.hlsli>

#define NOISE_SCALE float2(1920.0f / 4.0f, 1080.0f / 4.0f)
#define SAMPLE_KERNEL_SIZE 16U
#define RADIUS 1.0f

struct Input {
	float4 mPosH : SV_POSITION;
	float3 mViewRayV : VIEW_RAY;
	float2 mTexCoordO : TEXCOORD;
};

ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b0);
ConstantBuffer<ImmutableCBuffer> gImmutableCBuffer : register(b1);

SamplerState TexSampler : register (s0);

Texture2D<float4> Normal_Smoothness : register (t0);
Texture2D<float> Depth : register (t1);
Texture2D<float3> NoiseTexture : register (t2);
StructuredBuffer<float3> SampleKernel : register(t3);

struct Output {
	float mAccessibility : SV_Target0;
};

Output main(const in Input input) {
	Output output = (Output)0;

	const int3 screenCoord = int3(input.mPosH.xy, 0);
	
	// Clamp the view space position to the plane at Z = 1
	const float3 viewRay = float3(input.mViewRayV.xy / input.mViewRayV.z, 1.0f);

	// Sample the depth and convert to linear view space Z (assume it gets sampled as
	// a floating point value of the range [0,1])
	const float depth = Depth.Load(screenCoord);
	const float linearDepth = gImmutableCBuffer.mProjectionA_ProjectionB.y / (depth - gImmutableCBuffer.mProjectionA_ProjectionB.x);
	const float3 geomPosV = viewRay * linearDepth;

	// Get normal
	const float2 normal = Normal_Smoothness.Load(screenCoord).xy;
	const float3 normalV = normalize(Decode(normal));

	// Construct a change-of-basis matrix to reorient our sam ple kernel
	// along the origin's normal.
	const float3 rvec = NoiseTexture.Sample(TexSampler, input.mTexCoordO * NOISE_SCALE).xyz * 2.0 - 1.0;
	const float3 tangentV = normalize(rvec - normalV * dot(rvec, normalV));
	const float3 bitangentV = cross(normalV, tangentV);
	const float3x3 tbn = float3x3(tangentV, bitangentV, normalV);
	
	float occlusion = 0.0f;
	for (uint i = 0U; i < SAMPLE_KERNEL_SIZE; ++i) {
		// Get sample position:
		//float3 samplePos = mul(SampleKernel[i], tbn);
		float3 samplePos = SampleKernel[i];
		samplePos = samplePos * RADIUS + geomPosV;

		// Project sample position:
		float4 offset = float4(samplePos, 1.0);
		offset = mul(offset, gFrameCBuffer.mP);
		offset.xy /= offset.w;
		offset.xy = offset.xy * 0.5 + 0.5;

		// Get sample depth:
		const float sampleDepth = Depth.Load(float3(offset.xy, 0));

		// range check & accumulate:
		const float rangeCheck = abs(geomPosV.z - sampleDepth) < RADIUS ? 1.0 : 0.0;
		occlusion += (sampleDepth <= samplePos.z ? 1.0 : 0.0) * rangeCheck;
	}

	output.mAccessibility = 1.0f - occlusion;

	return output;
}
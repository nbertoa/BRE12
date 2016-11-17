#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Utils.hlsli>

#define SAMPLE_KERNEL_SIZE 16U
#define NOISE_SCALE float2(1920.0f / 4.0f, 1080.0f / 4.0f)
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
StructuredBuffer<float3> SampleKernel : register(t2);
Texture2D<float3> NoiseTexture : register (t3); 

struct Output {
	float mAccessibility : SV_Target0;
};

Output main(const in Input input) {
	Output output = (Output)0;

	float3 kernelArr[16] = {
		float3(-0.0974135324, 0.0124192238, 0.0188776907),
		float3(0.0798695162, 0.0219914615, 0.0620702952),
		float3(-0.0289274212, 0.0765098035, 0.0794965997),
		float3(0.0547583252, 0.0723639354, 0.0953637287),
		float3(0.126505822, 0.00813415647, 0.0913464576),
		float3(-0.138107583, -0.116347536, 0.0518886447),
		float3(-0.115297005, -0.109221123, 0.161579430),
		float3(-0.0384278893, -0.269535035, 0.00165199942),
		float3(-0.279213846, -0.0694325715, 0.151141420),
		float3(0.0835033357, 0.119374909, 0.356119931),
		float3(-0.346876413, 0.169449717, 0.234248236),
		float3(-0.139451042, -0.417720020, 0.286528677),
		float3(0.351039588, 0.374937564, 0.322074592),
		float3(-0.245884880, 0.466781467, 0.451095194),
		float3(0.529360473, 0.494314313, 0.313130111),
		float3(-0.842130244, -0.0892825574, 0.277045369),
	};

	float3 noises[16] = {
		float3(-0.0974135324, 0.0124192238, 0.0),
		float3(0.0798695162, 0.0219914615, 0.0),
		float3(-0.0289274212, 0.0765098035, 0.0),
		float3(0.0547583252, 0.0723639354, 0.0),
		float3(0.126505822, 0.00813415647, 0.0),
		float3(-0.138107583, -0.116347536, 0.0),
		float3(-0.115297005, -0.109221123, 0.0),
		float3(-0.0384278893, -0.269535035, 0.0),
		float3(-0.279213846, -0.0694325715, 0.0),
		float3(0.0835033357, 0.119374909, 0.0),
		float3(-0.346876413, 0.169449717, 0.0),
		float3(-0.139451042, -0.417720020, 0.0),
		float3(0.351039588, 0.374937564, 0.0),
		float3(-0.245884880, 0.466781467, 0.0),
		float3(0.529360473, 0.494314313, 0.0),
		float3(-0.842130244, -0.0892825574, 0.0),
	};

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
	float3 rvec = NoiseTexture.Sample(TexSampler, input.mTexCoordO * NOISE_SCALE).xyz * 2.0 - 1.0;
	rvec = noises[0];
	rvec = normalize(rvec);
	const float3 tangentV = normalize(rvec - normalV * dot(rvec, normalV));
	const float3 bitangentV = cross(normalV, tangentV);
	const float3x3 tbn = float3x3(tangentV, bitangentV, normalV);
	
	float occlusion = 0.0f;
	for (uint i = 0U; i < SAMPLE_KERNEL_SIZE; ++i) {
		// Get sample position:
		float3 samplePos = kernelArr[i];//mul(SampleKernel[i], tbn);
		samplePos = samplePos * RADIUS + geomPosV;

		// Project sample position:
		float4 offset = float4(samplePos, 1.0f);
		offset = mul(offset, gFrameCBuffer.mP);
		offset.xy /= offset.w;
		offset.xy = offset.xy * 0.5f + float2(0.5f, 0.5f);

		// Get sample depth:
		float sampleDepth = Depth.Load(float3(offset.xy, 0));
		sampleDepth = gImmutableCBuffer.mProjectionA_ProjectionB.y / (sampleDepth - gImmutableCBuffer.mProjectionA_ProjectionB.x);

		// range check & accumulate:
		const float rangeCheck = abs(geomPosV.z - sampleDepth) < RADIUS ? 1.0 : 0.0;
		occlusion += (sampleDepth <= samplePos.z ? 1.0 : 0.0) * rangeCheck;
	}

	output.mAccessibility = 1.0f - (occlusion / SAMPLE_KERNEL_SIZE);

	return output;
}
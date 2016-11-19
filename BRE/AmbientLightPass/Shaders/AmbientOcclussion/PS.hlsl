#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Utils.hlsli>

#define SAMPLE_KERNEL_SIZE 128U
#define NOISE_SCALE float2(1920.0f / 4.0f, 1080.0f / 4.0f)
#define OCCLUSION_RADIUS 10000.5f
#define SURFACE_EPSILON 0.05f
#define OCCLUSION_FADE_START 0.2f
#define OCCLUSION_FADE_END 10000000.0f

// Determines how much the sample point q occludes the point p as a function
// of distZ.
float OcclusionFunction(float distZ) {
	//
	// If depth(q) is "behind" depth(p), then q cannot occlude p.  Moreover, if 
	// depth(q) and depth(p) are sufficiently close, then we also assume q cannot
	// occlude p because q needs to be in front of p by Epsilon to occlude p.
	//
	// We use the following function to determine the occlusion.  
	// 
	//
	//       1.0     -------------\
		//               |           |  \
	//               |           |    \
	//               |           |      \ 
//               |           |        \
	//               |           |          \
	//               |           |            \
	//  ------|------|-----------|-------------|---------|--> zv
//        0     Eps          z0            z1        
//

	float occlusion = 0.0f;
	if (distZ > SURFACE_EPSILON)
	{
		float fadeLength = OCCLUSION_FADE_END - OCCLUSION_FADE_START;

		// Linearly decrease occlusion from 1 to 0 as distZ goes 
		// from gOcclusionFadeStart to gOcclusionFadeEnd.	
		occlusion = saturate((OCCLUSION_FADE_END - distZ) / fadeLength);
	}

	return occlusion;
}

struct Input {
	float4 mPosH : SV_POSITION;
	float3 mViewRayV : VIEW_RAY;
	float2 mTexCoordO : TEXCOORD;
};

ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b0);

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
	
	// Sample the depth and convert to linear view space Z (assume it gets sampled as
	// a floating point value of the range [0,1])
	const float depth = Depth.Load(screenCoord);
	const float depthV = NdcDepthToViewDepth(depth, gFrameCBuffer.mP);

	//
	// Reconstruct full view space position (x,y,z).
	// Find t such that p = t * ViewRayV.
	// p.z = t * ViewRayV.z
	// t = p.z / ViewRayV.z
	//
	const float3 geomPosV = (depthV / input.mViewRayV.z) * input.mViewRayV;

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
	
	float occlusionSum = 0.0f;
	for (uint i = 0U; i < SAMPLE_KERNEL_SIZE; ++i) {
		// Sample a point near geomPosV within the occlusion radius.
		const float3 sampleV = geomPosV + UnmapF1(SampleKernel[i]) * OCCLUSION_RADIUS;

		// Project sample
		float4 sampleH = mul(float4(sampleV, 1.0f), gFrameCBuffer.mP);
		sampleH /= sampleH.w;

		// Find the nearest depth value along the ray from the eye to q (this is not
		// the depth of q, as q is just an arbitrary point near p and might
		// occupy empty space).  To find the nearest depth we look it up in the depthmap.

		float sampleDepthV = Depth.Load(float3(sampleH.xy, 0));
		sampleDepthV = NdcDepthToViewDepth(sampleDepthV, gFrameCBuffer.mP);

		// Reconstruct full view space position r = (rx,ry,rz).  We know r
		// lies on the ray of q, so there exists a t such that r = t*q.
		// r.z = t*q.z ==> t = r.z / q.z

		float3 r = (sampleDepthV / sampleV.z) * sampleV;

		//
		// Test whether r occludes p.
		//   * The product dot(n, normalize(r - p)) measures how much in front
		//     of the plane(p,n) the occluder point r is.  The more in front it is, the
		//     more occlusion weight we give it.  This also prevents self shadowing where 
		//     a point r on an angled plane (p,n) could give a false occlusion since they
		//     have different depth values with respect to the eye.
		//   * The weight of the occlusion is scaled based on how far the occluder is from
		//     the point we are computing the occlusion of.  If the occluder r is far away
		//     from p, then it does not occlude it.
		// 

		float distZ = geomPosV.z - r.z;
		float dp = max(dot(normalV, normalize(r - geomPosV)), 0.0f);

		float occlusion = dp*OcclusionFunction(distZ);

		occlusionSum += occlusion;
	}

	output.mAccessibility = 1.0f - (occlusionSum / SAMPLE_KERNEL_SIZE);

	return output;
}
#ifndef AMBIENT_OCCLUSION
#define AMBIENT_OCCLUSION

#include <ShaderUtils/Utils.hlsli>

// Determines how much the sample point q occludes the point p as a function
// of distZ.
float OcclusionFunction(
	const float distZ,
	const float surfaceEpsilon,
	const float occlusionFadeStart,
	const float occlusionFadeEnd) 
{
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
	if (distZ > surfaceEpsilon) {
		float fadeLength = occlusionFadeEnd - occlusionFadeStart;

		// Linearly decrease occlusion from 1 to 0 as distZ goes 
		// from occlusionFadeStart to occlusionFadeEnd.	
		occlusion = saturate((occlusionFadeEnd - distZ) / fadeLength);
	}

	return occlusion;
}

// Returns ambient occlusion factor.
// sampleKernelMatrix: Change-of-basis matrix to reorient our sample kernel
// along the origin's normal.
float SSAOVersion1(
	const float3 sampleKernel,
	const float3x3 sampleKernelMatrix, 
	const float4x4 projMatrix,
	const float occlusionRadius,
	const float3 fragPosV,
	Texture2D<float> depthTex)
{
	// Get sample position
	float3 sampleV = mul(sampleKernel, sampleKernelMatrix);
	sampleV = sampleV * occlusionRadius + fragPosV;

	// Project sample position
	float4 sampleH = float4(sampleV, 1.0f);
	sampleH = mul(sampleH, projMatrix);
	sampleH.xy /= sampleH.w;
	const float x = (sampleH.x + 1.0f) * 1920.0f * 0.5f;
	const float y = (1.0f - sampleH.y) * 1080.0f * 0.5f;

	// Get sample depth
	float sampleDepthV = depthTex.Load(float3(x, y, 0));
	sampleDepthV = NdcDepthToViewDepth(sampleDepthV, projMatrix);

	// Range check and ambient occlusion factor
	const float rangeCheck = abs(fragPosV.z - sampleDepthV) < occlusionRadius ? 1.0 : 0.0;
	return (sampleDepthV <= sampleV.z ? 1.0 : 0.0) * rangeCheck;
}

float SSAOVersion2(
	const float3 sampleKernel, 
	const float3 noiseVec,
	const float3 normalV,
	const float4x4 projMatrix,
	const float occlusionRadius,
	const float3 fragPosV,
	Texture2D<float> depthTex,
	const float surfaceEpsilon,
	const float occlusionFadeStart,
	const float occlusionFadeEnd)
{
	const float3 offset = reflect(sampleKernel, noiseVec);

	const float flip = sign(dot(offset, normalV));

	// Sample a point near fragPosV within the occlusion radius.
	const float3 sampleV = fragPosV + flip * offset * occlusionRadius;

	// Project sample
	float4 sampleH = mul(float4(sampleV, 1.0f), projMatrix);
	sampleH /= sampleH.w;
	const float x = (sampleH.x + 1.0f) * 1920.0f * 0.5f;
	const float y = (1.0f - sampleH.y) * 1080.0f * 0.5f;

	// Find the nearest depth value along the ray from the eye to q (this is not
	// the depth of q, as q is just an arbitrary point near p and might
	// occupy empty space).  To find the nearest depth we look it up in the depthmap.
	float sampleDepthV = depthTex.Load(float3(x, y, 0));
	sampleDepthV = NdcDepthToViewDepth(sampleDepthV, projMatrix);

	// Reconstruct full view space position r = (rx,ry,rz).  We know r
	// lies on the ray of q, so there exists a t such that r = t*q.
	// r.z = t*q.z ==> t = r.z / q.z
	const float3 r = (sampleDepthV / sampleV.z) * sampleV;

	// Test whether r occludes p.
	//   * The product dot(n, normalize(r - p)) measures how much in front
	//     of the plane(p,n) the occluder point r is.  The more in front it is, the
	//     more occlusion weight we give it.  This also prevents self shadowing where 
	//     a point r on an angled plane (p,n) could give a false occlusion since they
	//     have different depth values with respect to the eye.
	//   * The weight of the occlusion is scaled based on how far the occluder is from
	//     the point we are computing the occlusion of.  If the occluder r is far away
	//     from p, then it does not occlude it.
	const float distZ = fragPosV.z - r.z;
	const float dp = max(dot(normalV, normalize(r - fragPosV)), 0.0f);

	return dp * OcclusionFunction(distZ, surfaceEpsilon, occlusionFadeStart, occlusionFadeEnd);
}

#endif
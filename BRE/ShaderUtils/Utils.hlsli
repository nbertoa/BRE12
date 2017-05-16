#ifndef UTILS_HEADER
#define UTILS_HEADER

//
// Space transformations
//

float NdcZToScreenSpaceZ(const float depthNDC, const float4x4 projection) {
	// depthNDC = A + B / depthV, where A = projection[2, 2] and B = projection[3,2].
	const float depthV = projection._m32 / (depthNDC - projection._m22);

	return depthV;
}

int2 NdcToScreenSpace(
	const float2 ndcPoint,
	const float screenTopLeftX,
	const float screenTopLeftY,
	const float screenWidth,
	const float screenHeight) {
	
	const int2 viewportPoint = 
		int2(
			(ndcPoint.x + 1.0f) * screenWidth * 0.5f + screenTopLeftX,
			(1.0f - ndcPoint.y) * screenHeight * 0.5f + screenTopLeftY
		);

	return viewportPoint;
}

float3 ViewRayToViewPosition(const float3 normalizedViewRayV, const float depthNDC, const float4x4 projection) {
	const float depthV = NdcZToScreenSpaceZ(depthNDC, projection);

	//
	// Reconstruct full view space position (x,y,z).
	// Find t such that p = t * normalizedViewRayV.
	// p.z = t * normalizedViewRayV.z
	// t = p.z / normalizedViewRayV.z
	//
	const float3 fragmentPositionViewSpace = (depthV / normalizedViewRayV.z) * normalizedViewRayV;

	return fragmentPositionViewSpace;
}

//
// Octahedron-normal vector encoding/decoding 
//
float2 OctWrap(float2 v) {
	return (1.0 - abs(v.yx)) * (v.xy >= 0.0 ? 1.0 : -1.0);
}

float2 Encode(float3 n) {
	n /= (abs(n.x) + abs(n.y) + abs(n.z));
	n.xy = n.z >= 0.0 ? n.xy : OctWrap(n.xy);
	n.xy = n.xy * 0.5 + 0.5;
	return n.xy;
}

float3 Decode(float2 encN) {
	encN = encN * 2.0 - 1.0;

	float3 n;
	n.z = 1.0 - abs(encN.x) - abs(encN.y);
	n.xy = n.z >= 0.0 ? encN.xy : OctWrap(encN.xy);
	n = normalize(n);
	return n;
}

#endif
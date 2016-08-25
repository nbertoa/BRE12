#ifndef UTILS_HEADER
#define UTILS_HEADER

//
// Octahedron-normal vectors 
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

// Map a normal from [-1.0f, 1.0f] to [0.0f, 1.0f]
float3 MapNormal(const float3 n) {
	return n * 0.5f + float3(0.5f, 0.5f, 0.5f);
}

// UnMap a normal from [0.0f, 1.0f] to [-1.0f, 1.0f]
float3 UnmapNormal(const float3 n) {
	return n * 2.0f - float3(1.0f, 1.0f, 1.0f);
}

#endif
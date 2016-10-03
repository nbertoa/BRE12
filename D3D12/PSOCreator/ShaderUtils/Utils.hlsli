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

//
// sRGB <-> Linear 
// 
float3 approximationSRgbToLinear(in float3 sRGBCol) {
	return pow(sRGBCol, 2.2);
}

float3 accurateSRGBToLinear(in float3 sRGBCol) {
	float3 linearRGBLo = sRGBCol / 12.92;
	float3 linearRGBHi = pow((sRGBCol + 0.055) / 1.055, 2.4);
	float3 linearRGB = (sRGBCol <= 0.04045) ? linearRGBLo : linearRGBHi;
	return linearRGB;
}

float3 HableToneMap(float3 color) {
	float A = 0.22; //Shoulder Strength
	float B = 0.30; //Linear Strength
	float C = 0.10; //Linear Angle
	float D = 0.20; //Toe Strength
	float E = 0.01; //Toe Numerator
	float F = 0.30; //Toe Denominator

	color = max(0, color - 0.004f);
	color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - (E / F);
	return color;
}

float3 approximationLinearToSRGB(in float3 linearCol) {
	return pow(linearCol, 1 / 2.2);
}

float3 accurateLinearToSRGB(in float3 linearCol) {
	const float3 sRGBLo = linearCol * 12.92;
	const float3 sRGBHi = (pow(abs(linearCol), 1.0 / 2.4) * 1.055) - 0.055;
	const float3 sRGB = (linearCol <= 0.0031308) ? sRGBLo : sRGBHi;
	return sRGB;
}

#endif
#include "../ShaderUtils/Lighting.hlsli"

#define QUAD_VERTICES (4)

struct Input {
	nointerpolation PunctualLight mPunctualLight : PUNCTUAL_LIGHT;
};

struct FrameConstants {
	float4x4 mV; // We do not need this, but we do this to reuse FrameConstant buffer used in VS
	float4x4 mP;
};
ConstantBuffer<FrameConstants> gFrameConstants : register(b0);

struct Output {
	float4 mPosH : SV_POSITION;
	float3 mPosV : VIEW_RAY;
	nointerpolation PunctualLight mPunctualLight : PUNCTUAL_LIGHT;
};

[maxvertexcount(QUAD_VERTICES)]
void main(const in point Input input[1], inout TriangleStream<Output> triangleStream) {
	// Compute quad center position in view space.
	// Then we can easily build a quad (two triangles) that face the camera.
	const float4 lightPosV = float4(input[0].mPunctualLight.mLightPosVAndRange.xyz, 1.0f);
	const float lightRange = input[0].mPunctualLight.mLightPosVAndRange.w;

	// Fix light z coordinate
	const float nearPlaneZ = 1.0f; // TODO: Pass near plane Z in a constant buffer
	const float lightMinZ = lightPosV.z - lightRange;
	const float lightMaxZ = lightPosV.z + lightRange;
	const float cond = lightMinZ < nearPlaneZ && nearPlaneZ < lightMaxZ;
	const float lightZ = cond * nearPlaneZ + (1.0f - cond) * lightMinZ;

	// Compute vertices positions and texture coordinates based on
	// a quad whose center position is lightCenterPosV
	Output output = (Output)0;

	float3 posV = float3(0.0f, 0.0f, lightZ);

	posV.xy = lightPosV.xy + float2(-lightRange, lightRange);
	output.mPosH = mul(float4(posV, 1.0f), gFrameConstants.mP);
	output.mPosV = posV;
	output.mPunctualLight = input[0].mPunctualLight;
	triangleStream.Append(output);

	posV.xy = lightPosV.xy + float2(lightRange, lightRange);
	output.mPosH = mul(float4(posV, 1.0f), gFrameConstants.mP);
	output.mPosV = posV;
	output.mPunctualLight = input[0].mPunctualLight;
	triangleStream.Append(output);

	posV.xy = lightPosV.xy + float2(-lightRange, -lightRange);
	output.mPosH = mul(float4(posV, 1.0f), gFrameConstants.mP);
	output.mPosV = posV;
	output.mPunctualLight = input[0].mPunctualLight;
	triangleStream.Append(output);

	posV.xy = lightPosV.xy + float2(lightRange, -lightRange);
	output.mPosH = mul(float4(posV, 1.0f), gFrameConstants.mP);
	output.mPosV = posV;
	output.mPunctualLight = input[0].mPunctualLight;
	triangleStream.Append(output);

	triangleStream.RestartStrip();
}
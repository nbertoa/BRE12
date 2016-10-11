#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Lights.hlsli>

#define QUAD_VERTICES (4)

struct Input {
	nointerpolation PunctualLight mPunctualLight : PUNCTUAL_LIGHT;
};

ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b0);
ConstantBuffer<ImmutableCBuffer> gImmutableCBuffer : register(b1);

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
	const float nearZ = gImmutableCBuffer.mNearZ_FarZ_ScreenW_ScreenH.x;
	const float lightMinZ = lightPosV.z - lightRange;
	const float lightMaxZ = lightPosV.z + lightRange;
	const float cond = lightMinZ < nearZ && nearZ < lightMaxZ;
	const float lightZ = cond * nearZ + (1.0f - cond) * lightMinZ;

	// Compute vertices positions and texture coordinates based on
	// a quad whose center position is lightCenterPosV
	Output output = (Output)0;

	float3 posV = float3(0.0f, 0.0f, lightZ);

	posV.xy = lightPosV.xy + float2(-lightRange, lightRange);
	output.mPosH = mul(float4(posV, 1.0f), gFrameCBuffer.mP);
	output.mPosV = posV;
	output.mPunctualLight = input[0].mPunctualLight;
	triangleStream.Append(output);

	posV.xy = lightPosV.xy + float2(lightRange, lightRange);
	output.mPosH = mul(float4(posV, 1.0f), gFrameCBuffer.mP);
	output.mPosV = posV;
	output.mPunctualLight = input[0].mPunctualLight;
	triangleStream.Append(output);

	posV.xy = lightPosV.xy + float2(-lightRange, -lightRange);
	output.mPosH = mul(float4(posV, 1.0f), gFrameCBuffer.mP);
	output.mPosV = posV;
	output.mPunctualLight = input[0].mPunctualLight;
	triangleStream.Append(output);

	posV.xy = lightPosV.xy + float2(lightRange, -lightRange);
	output.mPosH = mul(float4(posV, 1.0f), gFrameCBuffer.mP);
	output.mPosV = posV;
	output.mPunctualLight = input[0].mPunctualLight;
	triangleStream.Append(output);

	triangleStream.RestartStrip();
}
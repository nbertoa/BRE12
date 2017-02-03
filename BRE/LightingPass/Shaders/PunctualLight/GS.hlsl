#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Lights.hlsli>

#include "RS.hlsl"

#define QUAD_VERTICES (4)

struct Input {
	nointerpolation PunctualLight mPunctualLight : PUNCTUAL_LIGHT;
};

ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b0);
ConstantBuffer<ImmutableCBuffer> gImmutableCBuffer : register(b1);

struct Output {
	float4 mPositionClipSpace : SV_POSITION;
	float3 mCameraToFragmentViewSpace : VIEW_RAY;
	nointerpolation PunctualLight mPunctualLight : PUNCTUAL_LIGHT;
};

[RootSignature(RS)]
[maxvertexcount(QUAD_VERTICES)]
void main(const in point Input input[1], inout TriangleStream<Output> triangleStream) {
	// Compute quad center position in view space.
	// Then we can easily build a quad (two triangles) that face the camera.
	const float4 lightPositionViewSpace = float4(input[0].mPunctualLight.mLightPosVAndRange.xyz, 1.0f);
	const float lightRange = input[0].mPunctualLight.mLightPosVAndRange.w;

	// Fix light z coordinate
	const float nearZ = gImmutableCBuffer.mNearZ_FarZ_ScreenW_ScreenH.x;
	const float lightMinZ = lightPositionViewSpace.z - lightRange;
	const float lightMaxZ = lightPositionViewSpace.z + lightRange;
	const float cond = lightMinZ < nearZ && nearZ < lightMaxZ;
	const float lightZ = cond * nearZ + (1.0f - cond) * lightMinZ;

	// Compute vertices positions and texture coordinates based on
	// a quad whose center position is lightCenterPosV
	Output output = (Output)0;

	float3 positionViewSpace = float3(0.0f, 0.0f, lightZ);

	positionViewSpace.xy = lightPositionViewSpace.xy + float2(-lightRange, lightRange);
	output.mPositionClipSpace = mul(float4(positionViewSpace, 1.0f), gFrameCBuffer.mProjectionMatrix);
	output.mCameraToFragmentViewSpace = positionViewSpace;
	output.mPunctualLight = input[0].mPunctualLight;
	triangleStream.Append(output);

	positionViewSpace.xy = lightPositionViewSpace.xy + float2(lightRange, lightRange);
	output.mPositionClipSpace = mul(float4(positionViewSpace, 1.0f), gFrameCBuffer.mProjectionMatrix);
	output.mCameraToFragmentViewSpace = positionViewSpace;
	output.mPunctualLight = input[0].mPunctualLight;
	triangleStream.Append(output);

	positionViewSpace.xy = lightPositionViewSpace.xy + float2(-lightRange, -lightRange);
	output.mPositionClipSpace = mul(float4(positionViewSpace, 1.0f), gFrameCBuffer.mProjectionMatrix);
	output.mCameraToFragmentViewSpace = positionViewSpace;
	output.mPunctualLight = input[0].mPunctualLight;
	triangleStream.Append(output);

	positionViewSpace.xy = lightPositionViewSpace.xy + float2(lightRange, -lightRange);
	output.mPositionClipSpace = mul(float4(positionViewSpace, 1.0f), gFrameCBuffer.mProjectionMatrix);
	output.mCameraToFragmentViewSpace = positionViewSpace;
	output.mPunctualLight = input[0].mPunctualLight;
	triangleStream.Append(output);

	triangleStream.RestartStrip();
}
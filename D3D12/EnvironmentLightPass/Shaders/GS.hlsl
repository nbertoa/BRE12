#include <ShaderUtils/CBuffers.hlsli>

#define QUAD_VERTICES (4)

struct Input {
	uint mVertexId : DUMMY;
};

ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b0);
ConstantBuffer<ImmutableCBuffer> gImmutableCBuffer : register(b1);

struct Output {
	float4 mPosH : SV_POSITION;
	float3 mViewRayV : VIEW_RAY;
};

[maxvertexcount(QUAD_VERTICES)]
void main(const in point Input input[1], inout TriangleStream<Output> triangleStream) {
	const float nearZ = gImmutableCBuffer.mNearZ_FarZ_ScreenW_ScreenH.x;
	const float halfScreenWidth = gImmutableCBuffer.mNearZ_FarZ_ScreenW_ScreenH.z / 2.0f;
	const float halfScreenHeight = gImmutableCBuffer.mNearZ_FarZ_ScreenW_ScreenH.w / 2.0f;

	Output output = (Output)0;

	float3 posV = float3(0.0f, 0.0f, nearZ);

	posV.xy = float2(-halfScreenWidth, halfScreenHeight);
	output.mPosH = mul(float4(posV, 1.0f), gFrameCBuffer.mP);
	output.mViewRayV = posV;
	triangleStream.Append(output);

	posV.xy = float2(halfScreenWidth, halfScreenHeight);
	output.mPosH = mul(float4(posV, 1.0f), gFrameCBuffer.mP);
	output.mViewRayV = posV;
	triangleStream.Append(output);

	posV.xy = float2(-halfScreenWidth, -halfScreenHeight);
	output.mPosH = mul(float4(posV, 1.0f), gFrameCBuffer.mP);
	output.mViewRayV = posV;
	triangleStream.Append(output);

	posV.xy = float2(halfScreenWidth, -halfScreenHeight);
	output.mPosH = mul(float4(posV, 1.0f), gFrameCBuffer.mP);
	output.mViewRayV = posV;
	triangleStream.Append(output);

	triangleStream.RestartStrip();
}
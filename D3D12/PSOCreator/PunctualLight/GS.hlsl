#define QUAD_VERTICES (4)

struct Input {
	nointerpolation float4 mLightPosVAndRange : LIGHT_POSITION_AND_RANGE;
	nointerpolation float4 mLightColorAndPower : LIGHT_COLOR_AND_POWER;
};

struct FrameConstants {
	float4x4 mV; // TODO: Pass only projection
	float4x4 mP;
};
ConstantBuffer<FrameConstants> gFrameConstants : register(b0);

struct Output {
	float4 mPosH : SV_POSITION;
	nointerpolation float4 mLightPosVAndRange : LIGHT_POSITION_AND_RANGE;
	nointerpolation float4 mLightColorAndPower : LIGHT_COLOR_AND_POWER;
};

[maxvertexcount(QUAD_VERTICES)]
void main(const in point Input input[1], inout TriangleStream<Output> triangleStream) {
	// Compute quad center position in view space.
	// Then we can easily build a quad (two triangles) that face the camera.
	float4 lightPosV = float4(input[0].mLightPosVAndRange.xyz, 1.0f);
	const float lightRadius = input[0].mLightPosVAndRange.w;

	// Fix light z coordinate
	const float nearPlaneZ = 1.0f; // TODO: Pass near plane Z in a constant buffer
	const float lightMinZ = lightPosV.z - lightRadius;
	const float lightMaxZ = lightPosV.z + lightRadius;
	const float cond = lightMinZ < nearPlaneZ && nearPlaneZ < lightMaxZ;
	const float lightZ = cond * nearPlaneZ + (1.0f - cond) * lightMinZ;

	// Compute vertices positions and texture coordinates based on
	// a quad whose center position is lightCenterPosV
	Output output = (Output)0;

	float3 posV = float3(0.0f, 0.0f, lightZ);

	posV.xy = lightPosV.xy + float2(-lightRadius, lightRadius);
	output.mPosH = mul(float4(posV, 1.0f), gFrameConstants.mP);
	output.mLightPosVAndRange = input[0].mLightPosVAndRange;
	output.mLightColorAndPower = input[0].mLightColorAndPower;
	triangleStream.Append(output);

	posV.xy = lightPosV.xy + float2(lightRadius, lightRadius);
	output.mPosH = mul(float4(posV, 1.0f), gFrameConstants.mP);
	output.mLightPosVAndRange = input[0].mLightPosVAndRange;
	output.mLightColorAndPower = input[0].mLightColorAndPower;
	triangleStream.Append(output);

	posV.xy = lightPosV.xy + float2(-lightRadius, -lightRadius);
	output.mPosH = mul(float4(posV, 1.0f), gFrameConstants.mP);
	output.mLightPosVAndRange = input[0].mLightPosVAndRange;
	output.mLightColorAndPower = input[0].mLightColorAndPower;
	triangleStream.Append(output);

	posV.xy = lightPosV.xy + float2(lightRadius, -lightRadius);
	output.mPosH = mul(float4(posV, 1.0f), gFrameConstants.mP);
	output.mLightPosVAndRange = input[0].mLightPosVAndRange;
	output.mLightColorAndPower = input[0].mLightColorAndPower;
	triangleStream.Append(output);

	triangleStream.RestartStrip();
}
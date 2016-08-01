#define QUAD_VERTICES (4)

struct Input {
	nointerpolation float4 mLightPosVAndRange : LIGHT_POSITION_AND_RANGE;
	nointerpolation float4 mLightColorAndPower : LIGHT_COLOR_AND_POWER;
};

struct FrameConstants {
	float4x4 mV;
	float4x4 mP;
};
ConstantBuffer<FrameConstants> gFrameConstants : register(b0);

struct Output {
	float4 mPosH : SV_POSITION;
	nointerpolation float4 mLightPosVAndRange : LIGHT_POSITION_AND_RANGE;
	nointerpolation float4 mLightColorAndPower : LIGHT_COLOR_AND_POWER;
	float3 mPosV : POS_VIEW_SPACE;
};

[maxvertexcount(QUAD_VERTICES)]
void main(const in point Input input[1], inout TriangleStream<Output> triangleStream) {
	// Compute quad center position in view space.
	// Then we can easily build a quad (two triangles) that face the camera.
	const float4 lightCenterPosV = float4(input[0].mLightPosVAndRange.xyz, 1.0f);

	const float lightRadius = input[0].mLightPosVAndRange.w;

	// Compute vertices positions and texture coordinates based on
	// a quad whose center position is lightCenterPosV
	Output output = (Output)0;

	output.mPosV = lightCenterPosV.xyz + float3(-lightRadius, lightRadius, 0.0f);
	output.mPosH = mul(float4(output.mPosV, 1.0f), gFrameConstants.mP);
	output.mLightPosVAndRange = input[0].mLightPosVAndRange;
	output.mLightColorAndPower = input[0].mLightColorAndPower;
	triangleStream.Append(output);

	output.mPosV = lightCenterPosV.xyz + float3(lightRadius, lightRadius, 0.0f);
	output.mPosH = mul(float4(output.mPosV, 1.0f), gFrameConstants.mP);
	output.mLightPosVAndRange = input[0].mLightPosVAndRange;
	output.mLightColorAndPower = input[0].mLightColorAndPower;
	triangleStream.Append(output);

	output.mPosV = lightCenterPosV.xyz + float3(-lightRadius, -lightRadius, 0.0f);
	output.mPosH = mul(float4(output.mPosV, 1.0f), gFrameConstants.mP);
	output.mLightPosVAndRange = input[0].mLightPosVAndRange;
	output.mLightColorAndPower = input[0].mLightColorAndPower;
	triangleStream.Append(output);

	output.mPosV = lightCenterPosV.xyz + float3(lightRadius, -lightRadius, 0.0f);
	output.mPosH = mul(float4(output.mPosV, 1.0f), gFrameConstants.mP);
	output.mLightPosVAndRange = input[0].mLightPosVAndRange;
	output.mLightColorAndPower = input[0].mLightColorAndPower;
	triangleStream.Append(output);

	triangleStream.RestartStrip();
}
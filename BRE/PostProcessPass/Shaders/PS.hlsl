#include <ShaderUtils/Utils.hlsli>

#include "RS.hlsl"

#define FXAA_PC 1
#define FXAA_QUALITY__PRESET 12

#define SKIP_POST_PROCESS

#include <ShaderUtils/Fxaa.hlsli>

#define SCREEN_WIDTH 1920.0f
#define SCREEN_HEIGHT 1080.0f
#define RCP_FRAME float2(1.0f / SCREEN_WIDTH, 1.0f / SCREEN_HEIGHT)
#define QUALITY_SUB_PIX 1.0f
#define QUALITY_EDGE_THRESHOLD 0.125
#define QUALITY_EDGE_THRESHOLD_MIN 0.0625

struct Input {
	float4 mPositionScreenSpace : SV_POSITION;
	float2 mUV : TEXCOORD0;
};

SamplerState TextureSampler : register (s0);
Texture2D<float4> ColorBufferTexture : register(t0);

struct Output {
	float4 mColor : SV_Target0;
};

[RootSignature(RS)]
Output main(const in Input input){
	Output output = (Output)0;

	const int3 fragmentScreenSpace = int3(input.mPositionScreenSpace.xy, 0);

#ifdef SKIP_POST_PROCESS
	output.mColor = ColorBufferTexture.Load(fragmentScreenSpace);
#else
	FxaaTex tex = { TextureSampler, ColorBufferTexture };
	const float4 unusedFloat4 = { 0.0f, 0.0f, 0.0f, 0.0f };
	const float unusedFloat = 0.0f;
	output.mColor = FxaaPixelShader(
		input.mUV.xy,
		tex,
		RCP_FRAME,
		QUALITY_SUB_PIX,
		QUALITY_EDGE_THRESHOLD,
		QUALITY_EDGE_THRESHOLD_MIN);
#endif

	return output;
}
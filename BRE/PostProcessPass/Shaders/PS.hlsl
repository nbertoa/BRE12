#include <ShaderUtils/Utils.hlsli>

#include "RS.hlsl"

#define FXAA_PC 1
#define FXAA_QUALITY__PRESET 12

#include <ShaderUtils/Fxaa.hlsli>

#define RCP_FRAME float2(1.0f / 1920.0f, 1.0f / 1080.0f)
#define QUALITY_SUB_PIX 1.0f
#define QUALITY_EDGE_THRESHOLD 0.125
#define QUALITY_EDGE_THRESHOLD_MIN 0.0625

struct Input {
	float4 mPosH : SV_POSITION;
	float2 mTexCoordO : TEXCOORD0;
};

SamplerState TexSampler : register (s0);
Texture2D<float4> ColorBufferTexture : register(t0);

struct Output {
	float4 mColor : SV_Target0;
};

[RootSignature(RS)]
Output main(const in Input input){
	Output output = (Output)0;

	const int3 screenCoord = int3(input.mPosH.xy, 0);
	/*FxaaTex tex = { TexSampler, ColorBufferTexture };
	const float4 unusedFloat4 = { 0.0f, 0.0f, 0.0f, 0.0f };
	const float unusedFloat = 0.0f;
	output.mColor = FxaaPixelShader(
		input.mTexCoordO.xy, 
		tex,  
		RCP_FRAME,
		QUALITY_SUB_PIX,
		QUALITY_EDGE_THRESHOLD,
		QUALITY_EDGE_THRESHOLD_MIN);*/
	output.mColor = ColorBufferTexture.Load(screenCoord);
	return output;
}
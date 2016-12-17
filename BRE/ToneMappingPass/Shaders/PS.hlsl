#include <ShaderUtils/Utils.hlsli>

#define FXAA_PC 1
#define FXAA_HLSL_5 1
#define FXAA_QUALITY_PRESET 2
#define FXAA_GREEN_AS_LUMA 1

#include <ShaderUtils/Fxaa.hlsli>

struct Input {
	float4 mPosH : SV_POSITION;
	float2 mTexCoordO : TEXCOORD0;
};

SamplerState TexSampler : register (s0);
Texture2D<float4> ColorBufferTexture : register(t0);

struct Output {
	float4 mColor : SV_Target0;
};

Output main(const in Input input){
	Output output = (Output)0;

	const float4 rcpFrame = {1.0f / 1920.0f, 1.0f / 1080.0f, 0.0f, 0.0f };

	const int3 screenCoord = int3(input.mPosH.xy, 0);
	const float4 color = ColorBufferTexture.Load(screenCoord);
	output.mColor = float4(FilmicToneMapping(color.rgb), color.a);	
	output.mColor = float4(accurateLinearToSRGB(output.mColor.xyz), 1.0f);

	FxaaTex tex = { TexSampler, ColorBufferTexture };
	float4 ssss = { 0,0,0,0 };
	output.mColor = FxaaPixelShader(
		input.mTexCoordO.xy, 
		ssss, 
		tex, 
		tex, 
		tex, 
		rcpFrame.xy,
		ssss, 
		ssss, 
		ssss, 
		1.0f, 
		0.063, 
		0.0312, 
		0, 
		0, 
		0, 
		ssss);
	return output;
}
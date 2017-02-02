#include <ShaderUtils/Utils.hlsli>

#include "RS.hlsl"

#define SKIP_TONE_MAPPING

struct Input {
	float4 mPosH : SV_POSITION;
};

Texture2D<float4> ColorBufferTexture : register(t0);

struct Output {
	float4 mColor : SV_Target0;
};

[RootSignature(RS)]
Output main(const in Input input){
	Output output = (Output)0;

	const int3 screenCoord = int3(input.mPosH.xy, 0);

	const float4 color = ColorBufferTexture.Load(screenCoord);

#ifdef SKIP_TONE_MAPPING
	output.mColor = color;
#else 
	output.mColor = float4(FilmicToneMapping(color.rgb), color.a);
	output.mColor = float4(accurateLinearToSRGB(output.mColor.xyz), 1.0f);
	output.mColor.a = dot(output.mColor.rgb, float3(0.299, 0.587, 0.114)); // compute luma
#endif 

	return output;
}
#include <ShaderUtils/Utils.hlsli>

struct Input {
	float4 mPosH : SV_POSITION;
};

Texture2D<float4> ColorBufferTexture : register(t0);
Texture2D<float> DepthTexture : register(t1);

struct Output {
	float4 mColor : SV_Target0;
};

Output main(const in Input input){
	Output output = (Output)0;

	const int3 screenCoord = int3(input.mPosH.xy, 0);
	const float4 color = ColorBufferTexture.Load(screenCoord);
	output.mColor = float4(FilmicToneMapping(color.rgb), color.a);
	//const float depthNDC = DepthTexture.Load(screenCoord);
	//output.mColor = float4(depthNDC, depthNDC, depthNDC, 1.0f);
	
	output.mColor = float4(accurateLinearToSRGB(output.mColor.xyz), 1.0f);
	return output;
}
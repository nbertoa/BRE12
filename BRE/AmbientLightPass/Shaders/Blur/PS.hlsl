#include <ShaderUtils/Utils.hlsli>

#include "RS.hlsl"

// This should match noise texture dimension (for example, 4x4)
#define BLUR_SIZE 4

//#define SKIP_BLUR

struct Input {
	float4 mPosH : SV_POSITION;
	float2 mTexCoordO : TEXCOORD;
};

SamplerState TexSampler : register (s0);
Texture2D<float> BufferTexture : register(t0);

struct Output {
	float mColor : SV_Target0;
};

[RootSignature(RS)]
Output main(const in Input input){
	Output output = (Output)0;

#ifdef SKIP_BLUR
	const int3 screenCoord = int3(input.mPosH.xy, 0);
	output.mColor = BufferTexture.Load(screenCoord);
#else
	float w;
	float h;
	BufferTexture.GetDimensions(w, h);
	const float2 texelSize = 1.0 / float2(w, h);
	float result = 0.0;
	const float hlimComponent = float(-BLUR_SIZE) * 0.5 + 0.5;
	const float2 hlim = float2(hlimComponent, hlimComponent);
	for (uint i = 0; i < BLUR_SIZE; ++i) {
		for (uint j = 0; j < BLUR_SIZE; ++j) {
			const float2 offset = (hlim + float2(float(i), float(j))) * texelSize;
			result += BufferTexture.Sample(TexSampler, input.mTexCoordO + offset).r;
		}
	}

	output.mColor = result / float(BLUR_SIZE * BLUR_SIZE);
#endif

	return output;
}
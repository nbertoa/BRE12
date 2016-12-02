#include "../../../ShaderUtils/Utils.hlsli"

#define BLUR_SIZE 4

struct Input {
	float4 mPosH : SV_POSITION;
	float2 mTexCoordO : TEXCOORD;
};

SamplerState TexSampler : register (s0);
Texture2D BufferTexture : register(t0);

struct Output {
	float mBlur : SV_Target0;
};

Output main(const in Input input){
	Output output = (Output)0;

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

	output.mBlur = result / float(BLUR_SIZE * BLUR_SIZE);
	//output.mBlur = BufferTexture.Load(int3(input.mPosH.xy, 0)).x;
	
	return output;
}
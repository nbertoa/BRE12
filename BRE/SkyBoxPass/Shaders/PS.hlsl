#include "RS.hlsl"

#define SKIP_SKYBOX 

struct Input {
	float4 mPosH : SV_POSITION;
	float3 mPosO : POS_VIEW;
};

SamplerState TexSampler : register (s0);
TextureCube CubeMap : register(t0);

struct Output {
	float4 mColor : SV_Target0;
};

[RootSignature(RS)]
Output main(const in Input input) {
	Output output = (Output)0;

#ifdef SKIP_SKYBOX
	output.mColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
#else
	output.mColor = CubeMap.Sample(TexSampler, input.mPosO);
#endif
	return output;
}
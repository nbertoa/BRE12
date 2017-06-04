#include <EnvironmentLightPass/Shaders/BlurCBuffer.hlsli>
#include <ShaderUtils/Utils.hlsli>

#include "RS.hlsl"

//#define SKIP_BLUR

struct Input {
    float4 mPositionNDC : SV_POSITION;
    float2 mUV : TEXCOORD;
};

ConstantBuffer<BlurCBuffer> gBlurCBuffer : register(b0);

SamplerState TextureSampler : register (s0);
Texture2D<float> BufferTexture : register(t0);

struct Output {
    float mColor : SV_Target0;
};

[RootSignature(RS)]
Output main(const in Input input)
{
    Output output = (Output)0;

#ifdef SKIP_BLUR
    const int3 fragmentScreenSpace = int3(input.mPositionNDC.xy, 0);
    output.mColor = BufferTexture.Load(fragmentScreenSpace);
#else
    float w;
    float h;
    BufferTexture.GetDimensions(w, h);
    const float2 texelSize = 1.0f / float2(w, h);
    float result = 0.0f;
    const float hlimComponent = -float(gBlurCBuffer.mNoiseTextureDimension) * 0.5f + 0.5f;
    const float2 hlim = float2(hlimComponent, hlimComponent);
    for (uint i = 0U; i < gBlurCBuffer.mNoiseTextureDimension; ++i) {
        for (uint j = 0U; j < gBlurCBuffer.mNoiseTextureDimension; ++j) {
            const float2 offset = (hlim + float2(float(i), float(j))) * texelSize;
            result += BufferTexture.Sample(TextureSampler, 
                                           input.mUV + offset).r;
        }
    }

    output.mColor = result / float(gBlurCBuffer.mNoiseTextureDimension * gBlurCBuffer.mNoiseTextureDimension);
#endif

    return output;
}
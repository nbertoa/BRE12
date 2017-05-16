#include "RS.hlsl"

struct Input {
    float4 mPositionScreenSpace : SV_POSITION;
    float2 mUV : TEXCOORD0;
};

Texture2D<float4> ColorBufferTexture : register(t0);

struct Output {
    float4 mColor : SV_Target0;
};

[RootSignature(RS)]
Output main(const in Input input)
{
    Output output = (Output)0;

    const int3 fragmentScreenSpace = int3(input.mPositionScreenSpace.xy, 0);
    output.mColor = ColorBufferTexture.Load(fragmentScreenSpace);

    return output;
}
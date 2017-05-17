#include "RS.hlsl"

float3 accurateLinearToSRGB(in float3 linearCol)
{
    const float3 sRGBLo = linearCol * 12.92;
    const float3 sRGBHi = (pow(abs(linearCol), 1.0 / 2.4) * 1.055) - 0.055;
    const float3 sRGB = (linearCol <= 0.0031308) ? sRGBLo : sRGBHi;
    return sRGB;
}

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
    output.mColor = float4(accurateLinearToSRGB(output.mColor.xyz),
                           1.0f);

    return output;
}
#include "RS.hlsl"

float3 FilmicToneMapping(float3 color)
{
    const float A = 0.15f; // Shoulder Strength
    const float B = 0.5f; // Linear Strength
    const float C = 0.1f; // Linear Angle
    const float D = 0.2f; // Toe Strength
    const float E = 0.02f; // Toe Numerator
    const float F = 0.3f; // Toe Denominator
    float3 linearWhite = float3(11.2f, 11.2f, 11.2f);

    color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - (E / F);
    linearWhite = ((linearWhite * (A * linearWhite + C * B) + D * E) / (linearWhite * (A * linearWhite + B) + D * F)) - (E / F);
    return color / linearWhite;
}

//#define SKIP_TONE_MAPPING

struct Input {
    float4 mPositionNDC : SV_POSITION;
};

Texture2D<float4> ColorBufferTexture : register(t0);

struct Output {
    float4 mColor : SV_Target0;
};

[RootSignature(RS)]
Output main(const in Input input)
{
    Output output = (Output)0;

    const int3 fragmentPositionScreenSpace = int3(input.mPositionNDC.xy, 0);

    const float4 color = ColorBufferTexture.Load(fragmentPositionScreenSpace);

#ifdef SKIP_TONE_MAPPING
    output.mColor = color;
#else 
    output.mColor = float4(FilmicToneMapping(color.rgb),
                           color.a);
#endif

    return output;
}
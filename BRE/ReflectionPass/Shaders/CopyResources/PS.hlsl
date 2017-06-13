#include "RS.hlsl"

struct Input {
    float4 mPositionNDC : SV_POSITION;
};

Texture2D<float> DepthBuffer : register(t0);

struct Output {
    float2 mHiZBufferMipLevel0 : SV_Target0;
};

[RootSignature(RS)]
Output main(const in Input input)
{
    Output output = (Output)0;

    // We store the minimum and maximum for each hi-z buffer level in r and g channels respectively.
    // At the upmost level, the minimum and maximum is the same because we are working
    // at pixel level.
    const int3 fragmentScreenSpace = int3(input.mPositionNDC.xy, 0);
    output.mHiZBufferMipLevel0.r = DepthBuffer.Load(fragmentScreenSpace);
    output.mHiZBufferMipLevel0.g = output.mHiZBufferMipLevel0.r;

    return output;
}
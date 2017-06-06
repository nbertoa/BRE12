#include "RS.hlsl"

struct Input {
    float4 mPositionNDC : SV_POSITION;
};

Texture2D<float> DepthBuffer : register(t0);

struct Output {
    float mHiZBufferMipLevel0 : SV_Target0;
};

[RootSignature(RS)]
Output main(const in Input input)
{
    Output output = (Output)0;

    const int3 fragmentScreenSpace = int3(input.mPositionNDC.xy, 0);
    output.mHiZBufferMipLevel0 = DepthBuffer.Load(fragmentScreenSpace);

    return output;
}
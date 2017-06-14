#include "RS.hlsl"

struct Input {
    float4 mPositionNDC : SV_POSITION;
};

Texture2D<float2> HierZBufferUpperLevel : register(t0);
Texture2D<float> VisibilityBufferUpperLevel : register(t0);

struct Output {
    float mVisibilityBufferLowerLevel : SV_Target0;
};

[RootSignature(RS)]
Output main(const in Input input)
{
    Output output = (Output)0;

    /*const int3 fragmentScreenSpace = int3(input.mPositionNDC.xy, 0);
    const float2 minMaxDepth0 = HierZBufferUpperLevel.Load(fragmentScreenSpace);
    const float2 minMaxDepth1 = HierZBufferUpperLevel.Load(fragmentScreenSpace + int3(0, -1, 0));
    const float2 minMaxDepth2 = HierZBufferUpperLevel.Load(fragmentScreenSpace + int3(-1, 0, 0));
    const float2 minMaxDepth3 = HierZBufferUpperLevel.Load(fragmentScreenSpace + int3(-1, -1, 0));

    // We store the minimum and maximum neighbors depth in the R and G channels respectively.
    output.mHierZBufferLowerLevel.r = min(min(minMaxDepth0.x, minMaxDepth1.x),
                                          min(minMaxDepth2.x, minMaxDepth3.x));

    output.mHierZBufferLowerLevel.g = max(max(minMaxDepth0.y, minMaxDepth1.y),
                                          max(minMaxDepth2.y, minMaxDepth3.y));*/

    return output;
}
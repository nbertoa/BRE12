#include "RS.hlsl"

struct Input {
    float4 mPositionNDC : SV_POSITION;
};

Texture2D<float> HierZBufferUpperLevel : register(t0);

struct Output {
    float mHierZBufferLowerLevel : SV_Target0;
};

[RootSignature(RS)]
Output main(const in Input input)
{
    Output output = (Output)0;

    float4 minDepth;

    const int3 fragmentScreenSpace = int3(input.mPositionNDC.xy, 0);
    minDepth.x = HierZBufferUpperLevel.Load(fragmentScreenSpace);
    minDepth.y = HierZBufferUpperLevel.Load(fragmentScreenSpace + int3(0, -1, 0));
    minDepth.z = HierZBufferUpperLevel.Load(fragmentScreenSpace + int3(-1, 0, 0));
    minDepth.w = HierZBufferUpperLevel.Load(fragmentScreenSpace + int3(-1, -1, 0));

    // Take the minimum of the four depth values and return it.
    output.mHierZBufferLowerLevel = min(min(minDepth.x, minDepth.y),
                                        min(minDepth.z, minDepth.w));

    return output;
}
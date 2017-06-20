#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Utils.hlsli>

#include "RS.hlsl"

struct Input {
    float4 mPositionNDC : SV_POSITION;
};

Texture2D<float2> HierZBufferUpperLevel : register(t0);
Texture2D<float2> HierZBufferLowerLevel : register(t1);
Texture2D<float> VisibilityBufferUpperLevel : register(t2);

ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b0);

struct Output {
    float mVisibilityBufferLowerLevel : SV_Target0;
};

[RootSignature(RS)]
Output main(const in Input input)
{
    Output output = (Output)0;

    const int3 fragmentPositionScreenSpace = int3(input.mPositionNDC.xy, 0);
    float4 upperMinZ;
    upperMinZ.x = HierZBufferUpperLevel.Load(fragmentPositionScreenSpace).x;
    upperMinZ.y = HierZBufferUpperLevel.Load(fragmentPositionScreenSpace + int3(0, -1, 0)).x;
    upperMinZ.z = HierZBufferUpperLevel.Load(fragmentPositionScreenSpace + int3(-1, 0, 0)).x;
    upperMinZ.w = HierZBufferUpperLevel.Load(fragmentPositionScreenSpace + int3(-1, -1, 0)).x;
    upperMinZ.x = NdcZToScreenSpaceZ(upperMinZ.x, gFrameCBuffer.mProjectionMatrix);
    upperMinZ.y = NdcZToScreenSpaceZ(upperMinZ.y, gFrameCBuffer.mProjectionMatrix);
    upperMinZ.z = NdcZToScreenSpaceZ(upperMinZ.z, gFrameCBuffer.mProjectionMatrix);
    upperMinZ.w = NdcZToScreenSpaceZ(upperMinZ.w, gFrameCBuffer.mProjectionMatrix);

    float2 lowerMinMaxZ = HierZBufferLowerLevel.Load(fragmentPositionScreenSpace);
    lowerMinMaxZ.x = NdcZToScreenSpaceZ(lowerMinMaxZ.x, gFrameCBuffer.mProjectionMatrix);
    lowerMinMaxZ.y = NdcZToScreenSpaceZ(lowerMinMaxZ.y, gFrameCBuffer.mProjectionMatrix);

    const float coarseVolume = 1.0f / (lowerMinMaxZ.y - lowerMinMaxZ.x);

    // Get the previous 4 fine transparency values.
    float4 visibility;
    visibility.x = VisibilityBufferUpperLevel.Load(fragmentPositionScreenSpace);
    visibility.y = VisibilityBufferUpperLevel.Load(fragmentPositionScreenSpace + int3(0, -1, 0));
    visibility.z = VisibilityBufferUpperLevel.Load(fragmentPositionScreenSpace + int3(-1, 0, 0));
    visibility.w = VisibilityBufferUpperLevel.Load(fragmentPositionScreenSpace + int3(-1, -1, 0));

    // Calculate the percentage of visibility relative to the
    // calculated coarse depth. Modulate with transparency of previous mip.
    float4 integration = upperMinZ * abs(coarseVolume) * visibility;

    // Data-parallel add using SIMD with a weight of 0.25f because
    // we derive the transparency from 4 pixels. 
    output.mVisibilityBufferLowerLevel = dot(0.25f, integration);

    return output;
}
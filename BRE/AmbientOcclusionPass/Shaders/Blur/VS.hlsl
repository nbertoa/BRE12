#include <ShaderUtils/CBuffers.hlsli>

#include "RS.hlsl"

struct Input {
    uint mVertexId : SV_VertexID;
};

static const float2 gQuadUVs[6] = {
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f)
};

ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b0);

struct Output {
    float4 mPositionNDC : SV_POSITION;
    float2 mUV : TEXCOORD;
};

[RootSignature(RS)]
Output main(in const Input input)
{
    Output output;

    output.mUV = gQuadUVs[input.mVertexId];

    // Quad covering screen in NDC space ([-1.0, 1.0] x [-1.0, 1.0] x [0.0, 1.0] x [1.0])
    output.mPositionNDC = float4(2.0f * output.mUV.x - 1.0f,
                                 1.0f - 2.0f * output.mUV.y,
                                 0.0f,
                                 1.0f);

    return output;
}
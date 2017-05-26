#include <GeometryPass/Shaders/HeightMappingCBuffer.hlsli>
#include <ShaderUtils/CBuffers.hlsli>

#include "RS.hlsl"

#define NUM_PATCH_POINTS 3

struct HullShaderConstantOutput {
    float mEdgeFactors[3] : SV_TessFactor;
    float mInsideFactors : SV_InsideTessFactor;
};

struct Input {
    float3 mPositionWorldSpace : POS_WORLD;
    float3 mNormalWorldSpace : NORMAL_WORLD;
    float3 mTangentWorldSpace : TANGENT_WORLD;
    float2 mUV : TEXCOORD0;
};

ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b0);
ConstantBuffer<HeightMappingCBuffer> gHeightMappingCBuffer : register(b1);

SamplerState TextureSampler : register (s0);
Texture2D HeightTexture : register (t0);

struct Output {
    float4 mPositionClipSpace : SV_Position;
    float3 mPositionWorldSpace : POS_WORLD;
    float3 mPositionViewSpace : POS_VIEW;
    float3 mNormalWorldSpace : NORMAL_WORLD;
    float3 mNormalViewSpace : NORMAL_VIEW;
    float3 mTangentWorldSpace : TANGENT_WORLD;
    float3 mTangentViewSpace : TANGENT_VIEW;
    float3 mBinormalWorldSpace : BINORMAL_WORLD;
    float3 mBinormalViewSpace : BINORMAL_VIEW;
    float2 mUV : TEXCOORD0;
};

[RootSignature(RS)]
[domain("tri")]
Output main(const HullShaderConstantOutput HSConstantOutput,
            const float3 uvw : SV_DomainLocation,
            const OutputPatch <Input, NUM_PATCH_POINTS> patch)
{
    Output output = (Output)0;

    // Get texture coordinates
    output.mUV = uvw.x * patch[0].mUV + uvw.y * patch[1].mUV + uvw.z * patch[2].mUV;

    // Get normal
    const float3 normalWorldSpace =
        normalize(uvw.x * patch[0].mNormalWorldSpace + uvw.y * patch[1].mNormalWorldSpace + uvw.z * patch[2].mNormalWorldSpace);
    output.mNormalWorldSpace = normalize(normalWorldSpace);
    output.mNormalViewSpace = normalize(mul(float4(output.mNormalWorldSpace, 0.0f),
                                            gFrameCBuffer.mViewMatrix).xyz);

    // Get position
    float3 positionWorldSpace =
        uvw.x * patch[0].mPositionWorldSpace + uvw.y * patch[1].mPositionWorldSpace + uvw.z * patch[2].mPositionWorldSpace;
    float3 positionViewSpace = mul(float4(positionWorldSpace, 1.0f),
                                   gFrameCBuffer.mViewMatrix).xyz;

    const float height = HeightTexture.SampleLevel(TextureSampler,
                                                   output.mUV,
                                                   0).x;
    const float displacement = (gHeightMappingCBuffer.mHeightScale * (height - 1));

    // Offset vertex along normal
    positionWorldSpace += output.mNormalWorldSpace * displacement;
    positionViewSpace += output.mNormalViewSpace * displacement;

    // Get tangent
    output.mTangentWorldSpace =
        normalize(uvw.x * patch[0].mTangentWorldSpace + uvw.y * patch[1].mTangentWorldSpace + uvw.z * patch[2].mTangentWorldSpace);
    output.mTangentViewSpace = normalize(mul(float4(output.mTangentWorldSpace, 0.0f),
                                             gFrameCBuffer.mViewMatrix)).xyz;

    // Get binormal
    output.mBinormalWorldSpace = normalize(cross(output.mNormalWorldSpace,
                                                 output.mTangentWorldSpace));
    output.mBinormalViewSpace = normalize(cross(output.mNormalViewSpace,
                                                output.mTangentViewSpace));

    output.mPositionWorldSpace = positionWorldSpace;
    output.mPositionViewSpace = positionViewSpace;
    output.mPositionClipSpace = mul(float4(positionViewSpace, 1.0f),
                                    gFrameCBuffer.mProjectionMatrix);

    return output;
}
#include <ShaderUtils/CBuffers.hlsli>

#include "RS.hlsl"

struct Input {
    float3 mPositionObjectSpace : POSITION;
    float3 mNormalObjectSpace : NORMAL;
    float3 mTangentObjectSpace : TANGENT;
    float2 mUV : TEXCOORD;
};

ConstantBuffer<ObjectCBuffer> gObjCBuffer : register(b0);
ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b1);

struct Output {
    float4 mPositionClipSpace : SV_POSITION;
    float3 mPositionObjectSpace : POS_VIEW;
};

[RootSignature(RS)]
Output main(in const Input input)
{
    const float4x4 viewProjectionMatrix = mul(gFrameCBuffer.mViewMatrix,
                                              gFrameCBuffer.mProjectionMatrix);

    Output output;

    // Use local vertex position as cubemap lookup vector.
    output.mPositionObjectSpace = input.mPositionObjectSpace;

    // Always center sky about camera.
    float3 positionWorldSpace = mul(float4(input.mPositionObjectSpace, 1.0f),
                                    gObjCBuffer.mWorldMatrix).xyz;
    positionWorldSpace += gFrameCBuffer.mEyePositionWorldSpace.xyz;

    // Set z = w so that z/w = 1 (i.e., skydome always on far plane).
    output.mPositionClipSpace = mul(float4(positionWorldSpace, 1.0f),
                                    viewProjectionMatrix).xyww;

    return output;
}
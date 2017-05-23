#include <GeometryPass/Shaders/HeightMappingCBuffer.hlsli>
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
ConstantBuffer<HeightMappingCBuffer> gHeightMappingCBuffer : register(b2);

struct Output {
    float3 mPositionWorldSpace : POS_WORLD;
    float3 mNormalWorldSpace : NORMAL_WORLD;
    float3 mTangentWorldSpace : TANGENT_WORLD;
    float2 mUV : TEXCOORD0;
    float mTessellationFactor : TESS;
};

[RootSignature(RS)]
Output main(in const Input input)
{
    Output output;

    output.mPositionWorldSpace = mul(float4(input.mPositionObjectSpace, 1.0f),
                                     gObjCBuffer.mWorldMatrix).xyz;

    output.mNormalWorldSpace = mul(float4(input.mNormalObjectSpace, 0.0f),
                                   gObjCBuffer.mInverseTransposeWorldMatrix).xyz;

    output.mTangentWorldSpace = mul(float4(input.mTangentObjectSpace, 0.0f),
                                    gObjCBuffer.mWorldMatrix).xyz;

    output.mUV = gObjCBuffer.mTexTransform * input.mUV;

    // Normalized tessellation factor. 
    // The tessellation is 
    //   0 if d >= min tessellation distance and
    //   1 if d <= max tessellation distance.  
    const float distance = length(output.mPositionWorldSpace - gFrameCBuffer.mEyePositionWorldSpace.xyz);
    const float tessellationFactor = saturate((gHeightMappingCBuffer.mMinTessellationDistance - distance) 
                                              / (gHeightMappingCBuffer.mMinTessellationDistance 
                                                 - gHeightMappingCBuffer.mMaxTessellationDistance));

    // Rescale [0,1] --> [min tessellation factor, max tessellation factor].
    output.mTessellationFactor = gHeightMappingCBuffer.mMinTessellationFactor 
        + tessellationFactor * (gHeightMappingCBuffer.mMaxTessellationFactor - gHeightMappingCBuffer.mMinTessellationFactor);

    return output;
}
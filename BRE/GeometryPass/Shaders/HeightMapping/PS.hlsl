#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/MaterialProperties.hlsli>
#include <ShaderUtils/Utils.hlsli>

#include "RS.hlsl"

struct Input {
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

ConstantBuffer<MaterialProperties> gMaterialPropertiesCBuffer : register(b0);
ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b1);

SamplerState TextureSampler : register (s0);
Texture2D DiffuseTexture : register (t0);
Texture2D MetalnessTexture : register (t1);
Texture2D NormalTexture : register (t2);

struct Output {
    float4 mNormal_Smoothness : SV_Target0;
    float4 mBaseColor_MetalMask : SV_Target1;
};

[RootSignature(RS)]
Output main(const in Input input)
{
    Output output = (Output)0;

    // Normal (encoded in view space) 
    const float3 normalObjectSpace = normalize(NormalTexture.Sample(TextureSampler, 
                                                                    input.mUV).xyz * 2.0f - 1.0f);
    const float3x3 tbnWorldSpace = float3x3(normalize(input.mTangentWorldSpace),
                                            normalize(input.mBinormalWorldSpace),
                                            normalize(input.mNormalWorldSpace));
    const float3 normalWorldSpace = mul(normalObjectSpace, tbnWorldSpace);
    const float3x3 tbnViewSpace = float3x3(normalize(input.mTangentViewSpace),
                                           normalize(input.mBinormalViewSpace),
                                           normalize(input.mNormalViewSpace));
    output.mNormal_Smoothness.xy = Encode(normalize(mul(normalObjectSpace, tbnViewSpace)));

    // Base color and metalness
    const float3 diffuseColor = DiffuseTexture.Sample(TextureSampler,
                                                      input.mUV).rgb;
    const float metalness = MetalnessTexture.Sample(TextureSampler,
                                                    input.mUV).r;
    output.mBaseColor_MetalMask = float4(diffuseColor,
                                         metalness);

    // Smoothness
    output.mNormal_Smoothness.z = gMaterialPropertiesCBuffer.mMetalnessSmoothness.g;

    return output;
}
#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/MaterialProperties.hlsli>
#include <ShaderUtils/Utils.hlsli>

#include "RS.hlsl"

struct Input {
    float4 mPositionClipSpace : SV_POSITION;
    float3 mPositionWorldSpace : POS_WORLD;
    float3 mPositionViewSpace : POS_VIEW;
    float3 mNormalWorldSpace : NORMAL_WORLD;
    float3 mNormalViewSpace : NORMAL_VIEW;
    float2 mUV : TEXCOORD;
};

ConstantBuffer<MaterialProperties> gMaterialPropertiesCBuffer : register(b0);
ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b1);

SamplerState TextureSampler : register (s0);
Texture2D DiffuseTexture : register (t0);
Texture2D MetalnessTexture : register (t1);

struct Output {
    float4 mNormal_Smoothness : SV_Target0;
    float4 mBaseColor_MetalMask : SV_Target1;
};

[RootSignature(RS)]
Output main(const in Input input)
{
    Output output = (Output)0;

    // Normal (encoded in view space)
    const float3 normalViewSpace = normalize(input.mNormalViewSpace);
    output.mNormal_Smoothness.xy = Encode(normalViewSpace);

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
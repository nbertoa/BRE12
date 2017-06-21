#include <ShaderUtils/CBuffers.hlsli>
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

ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b0);

SamplerState TextureSampler : register (s0);
Texture2D BaseColorTexture : register (t0);
Texture2D MetalnessTexture : register (t1);
Texture2D RoughnessTexture : register (t2);

struct Output {
    float4 mNormal_Roughness : SV_Target0;
    float4 mBaseColor_Metalness : SV_Target1;
};

[RootSignature(RS)]
Output main(const in Input input)
{
    Output output = (Output)0;

    // Normal (encoded in view space)
    const float3 normalViewSpace = normalize(input.mNormalViewSpace);
    output.mNormal_Roughness.xy = Encode(normalViewSpace);

    // Base color and metalness
    const float3 baseColor = BaseColorTexture.Sample(TextureSampler,
                                                     input.mUV).rgb;

    const float metalness = MetalnessTexture.Sample(TextureSampler,
                                                    input.mUV).r;
    output.mBaseColor_Metalness = float4(baseColor,
                                         metalness);

    // Roughness
    output.mNormal_Roughness.z = RoughnessTexture.Sample(TextureSampler,
                                                          input.mUV).r;

    return output;
}
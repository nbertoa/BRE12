#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Lighting.hlsli>
#include <ShaderUtils/Utils.hlsli>

#include "RS.hlsl"

//#define SKIP_ENVIRONMENT_LIGHT 1
//#define DEBUG_AMBIENT_ACCESIBILITY 1
//#define SKIP_AMBIENT_ACCESSIBILITY 1

struct Input {
    float4 mPositionNDC : SV_POSITION;
    float3 mCameraToFragmentVectorViewSpace : VIEW_RAY;
};

ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b0);

SamplerState TextureSampler : register (s0);

Texture2D<float4> Normal_RoughnessTexture : register (t0);
Texture2D<float4> BaseColor_MetalnessTexture : register (t1);
TextureCube DiffuseIBLCubeMap : register(t2);
TextureCube SpecularIBLCubeMap : register(t3);
Texture2D<float> AmbientAccessibilityTexture : register (t4);
Texture2D<float> DepthTexture : register (t5);

struct Output {
    float4 mColor : SV_Target0;
};

[RootSignature(RS)]
Output main(const in Input input)
{
    Output output = (Output)0;

#ifdef SKIP_ENVIRONMENT_LIGHT
    output.mColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
#else
    const int3 fragmentPositionScreenSpace = int3(input.mPositionNDC.xy, 0);

    // Ambient accessibility (1.0f - ambient occlussion factor)
    const float ambientAccessibility = AmbientAccessibilityTexture.Load(fragmentPositionScreenSpace);

    const float4 normal_roughness = Normal_RoughnessTexture.Load(fragmentPositionScreenSpace);

    // Compute fragment position in view space
    const float fragmentZNDC = DepthTexture.Load(fragmentPositionScreenSpace);
    const float3 rayViewSpace = normalize(input.mCameraToFragmentVectorViewSpace);
    const float3 fragmentPositionViewSpace = ViewRayToViewPosition(rayViewSpace,
                                                                   fragmentZNDC,
                                                                   gFrameCBuffer.mProjectionMatrix);

    const float3 fragmentPositionWorldSpace = mul(float4(fragmentPositionViewSpace, 1.0f),
                                                  gFrameCBuffer.mInverseViewMatrix).xyz;

    const float2 encodedNormal = normal_roughness.xy;
    const float3 normalViewSpace = normalize(Decode(encodedNormal));
    const float3 normalWorldSpace = normalize(mul(float4(normalViewSpace, 0.0f),
                                                  gFrameCBuffer.mInverseViewMatrix).xyz);

    const float4 baseColor_metalness = BaseColor_MetalnessTexture.Load(fragmentPositionScreenSpace);
    const float3 baseColor = baseColor_metalness.xyz;
    const float metalness = baseColor_metalness.w;

    // As we are working at view space, we do not need camera position to 
    // compute vector from geometry position to camera.
    const float3 fragmentPositionToCameraViewSpace = normalize(-fragmentPositionViewSpace);

    const float3 indirectDiffuseColor = DiffuseIBL(baseColor,
                                                   metalness,
                                                   TextureSampler,
                                                   DiffuseIBLCubeMap,
                                                   normalWorldSpace);

    const float3 indirectSpecularColor = SpecularIBL(baseColor,
                                                     metalness,
                                                     normal_roughness.z,
                                                     TextureSampler,
                                                     SpecularIBLCubeMap,
                                                     fragmentPositionToCameraViewSpace,
                                                     fragmentPositionWorldSpace,
                                                     gFrameCBuffer.mEyePositionWorldSpace.xyz,
                                                     normalWorldSpace,
                                                     normalViewSpace);

    const float3 color = indirectDiffuseColor + indirectSpecularColor;

#ifdef DEBUG_AMBIENT_ACCESIBILITY
    output.mColor = float4(ambientAccessibility,
                           ambientAccessibility,
                           ambientAccessibility,
                           1.0f);
#elif SKIP_AMBIENT_ACCESSIBILITY
    output.mColor = float4(color, 1.0f);
#else 
    output.mColor = float4(color * ambientAccessibility, 1.0f);
#endif
#endif

    return output;
}
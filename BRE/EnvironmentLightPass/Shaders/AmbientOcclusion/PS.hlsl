#include <EnvironmentLightPass/Shaders/AmbientOcclusionCBuffer.hlsli>
#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Utils.hlsli>

#include "RS.hlsl"

#define SKIP_AMBIENT_OCCLUSION

struct Input {
    float4 mPositionNDC : SV_POSITION;
    float3 mRayViewSpace : VIEW_RAY;
    float2 mUV : TEXCOORD;
};

ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b0);
ConstantBuffer<AmbientOcclusionCBuffer> gAmbientOcclusionCBuffer : register(b1);

SamplerState TextureSampler : register (s0);

Texture2D<float4> Normal_SmoothnessTexture : register (t0);
StructuredBuffer<float4> SampleKernelBuffer : register(t1);
Texture2D<float4> NoiseTexture : register (t2);
Texture2D<float> DepthTexture : register (t3);

struct Output {
    float mAmbientAccessibility : SV_Target0;
};

[RootSignature(RS)]
Output main(const in Input input)
{
    Output output = (Output)0;

#ifdef SKIP_AMBIENT_OCCLUSION
    output.mAmbientAccessibility = 1.0f;
#else
    const float2 noiseScale = 
        float2(gAmbientOcclusionCBuffer.mScreenWidth / gAmbientOcclusionCBuffer.mNoiseTextureDimension, 
               gAmbientOcclusionCBuffer.mScreenHeight / gAmbientOcclusionCBuffer.mNoiseTextureDimension);

    const int3 fragmentPositionNDC = int3(input.mPositionNDC.xy, 0);

    const float fragmentZNDC = DepthTexture.Load(fragmentPositionNDC);
    const float3 rayViewSpace = normalize(input.mRayViewSpace);
    const float4 fragmentPositionViewSpace = float4(ViewRayToViewPosition(rayViewSpace,
                                                                          fragmentZNDC,
                                                                          gFrameCBuffer.mProjectionMatrix),
                                                    1.0f);

    const float2 normal = Normal_SmoothnessTexture.Load(fragmentPositionNDC).xy;
    const float3 normalViewSpace = normalize(Decode(normal));

    // Build a matrix to reorient the sample kerne
    // along current fragment normal vector.
    const float3 noiseVec = NoiseTexture.SampleLevel(TextureSampler, 
                                                     noiseScale * input.mUV, 
                                                     0).xyz * 2.0f - 1.0f;
    const float3 tangentViewSpace = normalize(noiseVec - normalViewSpace * dot(noiseVec, normalViewSpace));
    const float3 bitangentViewSpace = normalize(cross(normalViewSpace, tangentViewSpace));
    const float3x3 sampleKernelRotationMatrix = float3x3(tangentViewSpace,
                                                         bitangentViewSpace,
                                                         normalViewSpace);

    float occlusionSum = 0.0f;
    for (uint i = 0U; i < gAmbientOcclusionCBuffer.mSampleKernelSize; ++i) {
        // Rotate sample and get sample position in view space
        float4 rotatedSample = float4(mul(SampleKernelBuffer[i].xyz, sampleKernelRotationMatrix), 0.0f);
        float4 samplePositionViewSpace = 
            fragmentPositionViewSpace + rotatedSample * gAmbientOcclusionCBuffer.mOcclusionRadius;

        float4 samplePositionNDC = mul(samplePositionViewSpace, 
                                       gFrameCBuffer.mProjectionMatrix);
        samplePositionNDC.xy /= samplePositionNDC.w;

        const int2 samplePositionScreenSpace = NdcToScreenSpace(samplePositionNDC.xy,
                                                                0.0f,
                                                                0.0f,
                                                                gAmbientOcclusionCBuffer.mScreenWidth,
                                                                gAmbientOcclusionCBuffer.mScreenHeight);

        const bool isOutsideScreenBorders =
            samplePositionScreenSpace.x < 0.0f ||
            samplePositionScreenSpace.x > gAmbientOcclusionCBuffer.mScreenWidth ||
            samplePositionScreenSpace.y < 0.0f ||
            samplePositionScreenSpace.y > gAmbientOcclusionCBuffer.mScreenHeight;

        if (isOutsideScreenBorders == false) {
            float sampleZNDC = DepthTexture.Load(int3(samplePositionScreenSpace, 0));

            const float sampleZViewSpace = NdcZToScreenSpaceZ(sampleZNDC,
                                                              gFrameCBuffer.mProjectionMatrix);

            const float rangeCheck = 
                abs(fragmentPositionViewSpace.z - sampleZViewSpace) < 
                gAmbientOcclusionCBuffer.mOcclusionRadius ? 1.0f : 0.0f;
            occlusionSum += (sampleZViewSpace <= samplePositionViewSpace.z ? 1.0f : 0.0f) * rangeCheck;
        }
    }

    output.mAmbientAccessibility = 1.0f - (occlusionSum / gAmbientOcclusionCBuffer.mSampleKernelSize);
#endif

    // Sharpen the contrast
    output.mAmbientAccessibility = saturate(pow(abs(output.mAmbientAccessibility), 
                                                gAmbientOcclusionCBuffer.mSsaoPower));

    return output;
}
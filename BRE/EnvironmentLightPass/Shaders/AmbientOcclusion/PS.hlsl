#include <ShaderUtils/CBuffers.hlsli>
#include <ShaderUtils/Utils.hlsli>

#include "RS.hlsl"

#define SAMPLE_KERNEL_SIZE 16U
#define SCREEN_TOP_LEFT_X 0.0f
#define SCREEN_TOP_LEFT_Y 0.0f
#define SCREEN_WIDTH 1920.0f
#define SCREEN_HEIGHT 1080.0f
#define NOISE_TEXTURE_DIMENSION 4.0f
#define NOISE_SCALE float2(SCREEN_WIDTH / NOISE_TEXTURE_DIMENSION, SCREEN_HEIGHT/ NOISE_TEXTURE_DIMENSION)
#define OCCLUSION_RADIUS 1.5f
#define SSAO_POWER 1.0f

//#define SKIP_AMBIENT_OCCLUSION

struct Input {
    float4 mPositionScreenSpace : SV_POSITION;
    float3 mRayViewSpace : VIEW_RAY;
    float2 mUV : TEXCOORD;
};

ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b0);

SamplerState TextureSampler : register (s0);

Texture2D<float4> Normal_SmoothnessTexture : register (t0);
Texture2D<float> DepthTexture : register (t1);
StructuredBuffer<float4> SampleKernelBuffer : register(t2);
Texture2D<float4> NoiseTexture : register (t3);

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
    const int3 fragmentScreenSpace = int3(input.mPositionScreenSpace.xy, 0);

    const float fragmentZNDC = DepthTexture.Load(fragmentScreenSpace);
    const float3 rayViewSpace = normalize(input.mRayViewSpace);
    const float4 fragmentPositionViewSpace = float4(ViewRayToViewPosition(rayViewSpace,
                                                                          fragmentZNDC,
                                                                          gFrameCBuffer.mProjectionMatrix),
                                                    1.0f);

    const float2 normal = Normal_SmoothnessTexture.Load(fragmentScreenSpace).xy;
    const float3 normalViewSpace = normalize(Decode(normal));

    // Build a matrix to reorient the sample kernel
    // along current fragment normal vector.
    const float3 noiseVec = NoiseTexture.SampleLevel(TextureSampler, NOISE_SCALE * input.mUV, 0.0f).xyz * 2.0f - 1.0f;
    const float3 tangentViewSpace = normalize(noiseVec - normalViewSpace * dot(noiseVec, normalViewSpace));
    const float3 bitangentViewSpace = normalize(cross(normalViewSpace, tangentViewSpace));
    const float3x3 sampleKernelRotationMatrix = float3x3(tangentViewSpace,
                                                         bitangentViewSpace,
                                                         normalViewSpace);

    float occlusionSum = 0.0f;
    for (uint i = 0U; i < SAMPLE_KERNEL_SIZE; ++i) {
        // Rotate sample and get sample position in view space
        float4 rotatedSample = float4(mul(SampleKernelBuffer[i].xyz, sampleKernelRotationMatrix), 0.0f);
        float4 samplePositionViewSpace = fragmentPositionViewSpace + rotatedSample * OCCLUSION_RADIUS;

        float4 samplePositionNDC = mul(samplePositionViewSpace, gFrameCBuffer.mProjectionMatrix);
        samplePositionNDC.xy /= samplePositionNDC.w;

        const int2 samplePositionScreenSpace = NdcToScreenSpace(samplePositionNDC.xy,
                                                                SCREEN_TOP_LEFT_X,
                                                                SCREEN_TOP_LEFT_Y,
                                                                SCREEN_WIDTH,
                                                                SCREEN_HEIGHT);

        const bool isOutsideScreenBorders =
            samplePositionScreenSpace.x < SCREEN_TOP_LEFT_X ||
            samplePositionScreenSpace.x > SCREEN_WIDTH ||
            samplePositionScreenSpace.y < SCREEN_TOP_LEFT_Y ||
            samplePositionScreenSpace.y > SCREEN_HEIGHT;

        if (isOutsideScreenBorders == false) {
            float sampleZNDC = DepthTexture.Load(int3(samplePositionScreenSpace, 0));

            const float sampleZViewSpace = NdcZToScreenSpaceZ(sampleZNDC,
                                                              gFrameCBuffer.mProjectionMatrix);

            const float rangeCheck = abs(fragmentPositionViewSpace.z - sampleZViewSpace) < OCCLUSION_RADIUS ? 1.0f : 0.0f;
            occlusionSum += (sampleZViewSpace <= samplePositionViewSpace.z ? 1.0f : 0.0f) * rangeCheck;
        }
    }

    output.mAmbientAccessibility = 1.0f - (occlusionSum / SAMPLE_KERNEL_SIZE);
#endif

    // Sharpen the contrast
    output.mAmbientAccessibility = saturate(pow(output.mAmbientAccessibility, SSAO_POWER));

    return output;
}
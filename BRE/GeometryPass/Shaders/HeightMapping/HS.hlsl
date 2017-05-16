#include "RS.hlsl"

#define NUM_PATCH_POINTS 3

struct Input {
    float3 mPositionWorldSpace : POS_WORLD;
    float3 mNormalWorldSpace : NORMAL_WORLD;
    float3 mTangentWorldSpace : TANGENT_WORLD;
    float2 mUV : TEXCOORD0;
    float mTessellationFactor : TESS;
};

struct HullShaderConstantOutput {
    float mEdgeFactors[3] : SV_TessFactor;
    float mInsideFactors : SV_InsideTessFactor;
};

struct Output {
    float3 mPositionWorldSpace : POS_WORLD;
    float3 mNormalWorldSpace : NORMAL_WORLD;
    float3 mTangentWorldSpace : TANGENT_WORLD;
    float2 mUV : TEXCOORD0;
};

HullShaderConstantOutput constant_hull_shader(const InputPatch<Input, NUM_PATCH_POINTS> patch,
                                              const uint patchID : SV_PrimitiveID)
{
    // Average tess factors along edges, and pick an edge tess factor for 
    // the interior tessellation. It is important to do the tess factor
    // calculation based on the edge properties so that edges shared by 
    // more than one triangle will have the same tessellation factor.  
    // Otherwise, gaps can appear.
    HullShaderConstantOutput output = (HullShaderConstantOutput)0;
    output.mEdgeFactors[0] = 0.5f * (patch[1].mTessellationFactor + patch[2].mTessellationFactor);
    output.mEdgeFactors[1] = 0.5f * (patch[2].mTessellationFactor + patch[0].mTessellationFactor);
    output.mEdgeFactors[2] = 0.5f * (patch[0].mTessellationFactor + patch[1].mTessellationFactor);
    output.mInsideFactors = output.mEdgeFactors[0];

    return output;
}

[RootSignature(RS)]
[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(NUM_PATCH_POINTS)]
[patchconstantfunc("constant_hull_shader")]
Output main(const InputPatch <Input, NUM_PATCH_POINTS> patch,
            const uint controlPointID : SV_OutputControlPointID,
            const uint patchId : SV_PrimitiveID)
{
    Output output = (Output)0;
    output.mPositionWorldSpace = patch[controlPointID].mPositionWorldSpace;
    output.mNormalWorldSpace = patch[controlPointID].mNormalWorldSpace;
    output.mTangentWorldSpace = patch[controlPointID].mTangentWorldSpace;
    output.mUV = patch[controlPointID].mUV;

    return output;
}
#define NUM_PATCH_POINTS 3

struct Input {
	float3 mPosW : POS_WORLD;
	float3 mNormalW : NORMAL_WORLD;
	float3 mTangentW : TANGENT_WORLD;
	float2 mTexCoordO : TEXCOORD0;
	float mTessFactor : TESS;
};

struct HullShaderConstantOutput {
	float mEdgeFactors[3] : SV_TessFactor;
	float mInsideFactors : SV_InsideTessFactor;
};

struct Output {
	float3 mPosW : POS_WORLD;
	float3 mNormalW : NORMAL_WORLD;
	float3 mTangentW : TANGENT_WORLD;
	float2 mTexCoordO : TEXCOORD0;
};

HullShaderConstantOutput constant_hull_shader(const InputPatch<Input, NUM_PATCH_POINTS> patch, const uint patchID : SV_PrimitiveID) {
	// Average tess factors along edges, and pick an edge tess factor for 
	// the interior tessellation. It is important to do the tess factor
	// calculation based on the edge properties so that edges shared by 
	// more than one triangle will have the same tessellation factor.  
	// Otherwise, gaps can appear.
	HullShaderConstantOutput output = (HullShaderConstantOutput)0;
	output.mEdgeFactors[0] = 0.5f * (patch[1].mTessFactor + patch[2].mTessFactor);
	output.mEdgeFactors[1] = 0.5f * (patch[2].mTessFactor + patch[0].mTessFactor);
	output.mEdgeFactors[2] = 0.5f * (patch[0].mTessFactor + patch[1].mTessFactor);
	output.mInsideFactors = output.mEdgeFactors[0];

	return output;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(NUM_PATCH_POINTS)]
[patchconstantfunc("constant_hull_shader")]
Output main(const InputPatch <Input, NUM_PATCH_POINTS> patch, const uint controlPointID : SV_OutputControlPointID, const uint patchId : SV_PrimitiveID) {
	Output output = (Output)0;
	output.mPosW = patch[controlPointID].mPosW;
	output.mNormalW = patch[controlPointID].mNormalW;
	output.mTangentW = patch[controlPointID].mTangentW;
	output.mTexCoordO = patch[controlPointID].mTexCoordO;
	
	return output;
}
#define NUM_PATCH_POINTS 3

struct Input {
	float4 mPosO : POSITION;
	float3 mNormalO : NORMAL;
	float2 mTexCoordO : TEXCOORD0;
	float3 mTangentO : TANGENT;
};

struct HullShaderConstantOutput {
	float mEdgeFactors[3] : SV_TessFactor;
	float mInsideFactors : SV_InsideTessFactor;
};

struct Output {
	float4 mPosO : POSITION;
	float3 mNormalO : NORMAL;
	float2 mTexCoordO : TEXCOORD0;
	float3 mTangentO : TANGENT;
};

HullShaderConstantOutput constant_hull_shader(const InputPatch<Input, NUM_PATCH_POINTS> patch, const uint patchID : SV_PrimitiveID) {
	const float TessellationFactor = 5.0f;
	HullShaderConstantOutput output = (HullShaderConstantOutput)0;
	output.mEdgeFactors[0] = TessellationFactor;
	output.mEdgeFactors[1] = TessellationFactor;
	output.mEdgeFactors[2] = TessellationFactor;
	output.mInsideFactors = TessellationFactor;
	return output;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(NUM_PATCH_POINTS)]
[patchconstantfunc("constant_hull_shader")]
Output main(const InputPatch <Input, NUM_PATCH_POINTS> patch, const uint controlPointID : SV_OutputControlPointID, const uint patchId : SV_PrimitiveID) {
	Output output = (Output)0;
	output.mPosO = patch[controlPointID].mPosO;
	output.mNormalO = patch[controlPointID].mNormalO;
	output.mTexCoordO = patch[controlPointID].mTexCoordO;
	output.mTangentO = patch[controlPointID].mTangentO;
	return output;
}
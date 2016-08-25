#define NUM_PATCH_POINTS 3

struct HullShaderConstantOutput {
	float mEdgeFactors[3] : SV_TessFactor;
	float mInsideFactors : SV_InsideTessFactor;
};

struct Input {
	float4 mPosO : POSITION;
	float3 mNormalO : NORMAL;
	float2 mTexCoordO : TEXCOORD0;
	float3 mTangentO : TANGENT;
};

struct ObjConstants {
	float4x4 mW;
};
ConstantBuffer<ObjConstants> gObjConstants : register(b0);

struct FrameConstants {
	float4x4 mV;
	float4x4 mP;
};
ConstantBuffer<FrameConstants> gFrameConstants : register(b1);

struct Output {
	float4 mPosH : SV_Position;
	float3 mPosV : POS_VIEW;
	float3 mNormalV : NORMAL;
	float2 mTexCoordO : TEXCOORD0;
	float3 mTangentV : TANGENT;
	float3 mBinormalV : BINORMAL;
};

SamplerState TexSampler : register (s0);
Texture2D HeightTexture : register (t0);

[domain("tri")]
Output main(const HullShaderConstantOutput IN, const float3 uvw : SV_DomainLocation, const OutputPatch <Input, NUM_PATCH_POINTS> patch) {
	const float4x4 wv = mul(gObjConstants.mW, gFrameConstants.mV);

	Output output = (Output)0;

	output.mTexCoordO = uvw.x * patch[0].mTexCoordO + uvw.y * patch[1].mTexCoordO + uvw.z * patch[2].mTexCoordO;

	const float3 normalO = normalize(uvw.x * patch[0].mNormalO + uvw.y * patch[1].mNormalO + uvw.z * patch[2].mNormalO);
	output.mNormalV = normalize(mul(float4(normalO, 0.0f), wv).xyz);

	// Compute SV_Position by displacing object position in y coordinate
	float4 posV = mul(uvw.x * patch[0].mPosO + uvw.y * patch[1].mPosO + uvw.z * patch[2].mPosO, wv);
	// Choose the mipmap level based on distance to the eye; specifically, choose the next miplevel every MipInterval units, and clamp the miplevel in [0,6].
	const float MipInterval = 20.0f;
	const float mipLevel = clamp((length(posV) - MipInterval) / MipInterval, 0.0f, 6.0f);
	const float DisplacementScale = 1.0f;
	const float displacement = (HeightTexture.SampleLevel(TexSampler, output.mTexCoordO, mipLevel).x - 1.0f) * DisplacementScale;
	// Offset vertex along normal
	posV += float4(output.mNormalV * displacement, 0.0f);
	output.mPosH = mul(posV, gFrameConstants.mP);

	// Compute world tangent and binormal
	output.mTangentV = normalize(uvw.x * patch[0].mTangentO + uvw.y * patch[1].mTangentO + uvw.z * patch[2].mTangentO);
	output.mTangentV = normalize(mul(float4(output.mTangentV, 0.0f), wv).xyz);
	output.mBinormalV = normalize(cross(output.mNormalV, output.mTangentV));
	output.mPosV = posV.xyz;

	return output;
}
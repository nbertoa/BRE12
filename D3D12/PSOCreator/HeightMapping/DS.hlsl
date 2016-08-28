#define NUM_PATCH_POINTS 3
#define HEIGHT_SCALE 0.2f

struct HullShaderConstantOutput {
	float mEdgeFactors[3] : SV_TessFactor;
	float mInsideFactors : SV_InsideTessFactor;
};

struct Input {
	float3 mPosV : POSITION;
	float3 mNormalV : NORMAL;
	float3 mTangentV : TANGENT;
	float2 mTexCoordO : TEXCOORD0;	
};

struct FrameConstants {
	float4x4 mV;
	float4x4 mP;
};
ConstantBuffer<FrameConstants> gFrameConstants : register(b1);

struct Output {
	float4 mPosH : SV_Position;
	float3 mPosV : POS_VIEW;
	float3 mNormalV : NORMAL;	
	float3 mTangentV : TANGENT;
	float3 mBinormalV : BINORMAL;
	float2 mTexCoordO : TEXCOORD0;
};

SamplerState TexSampler : register (s0);
Texture2D HeightTexture : register (t0);

[domain("tri")]
Output main(const HullShaderConstantOutput HSConstantOutput, const float3 uvw : SV_DomainLocation, const OutputPatch <Input, NUM_PATCH_POINTS> patch) {
	Output output = (Output)0;

	output.mTexCoordO = uvw.x * patch[0].mTexCoordO + uvw.y * patch[1].mTexCoordO + uvw.z * patch[2].mTexCoordO;
	output.mTexCoordO *= 2.0f;

	const float3 normalV = normalize(uvw.x * patch[0].mNormalV + uvw.y * patch[1].mNormalV + uvw.z * patch[2].mNormalV);
	output.mNormalV = normalize(normalV);

	float3 posV = uvw.x * patch[0].mPosV + uvw.y * patch[1].mPosV + uvw.z * patch[2].mPosV;

	// Choose the mipmap level based on distance to the eye; specifically, choose the next miplevel every MipInterval units, and clamp the miplevel in [0,6].
	const float MipInterval = 20.0f;
	const float mipLevel = clamp((length(posV) - MipInterval) / MipInterval, 0.0f, 6.0f);
	const float height = HeightTexture.SampleLevel(TexSampler, output.mTexCoordO, mipLevel).x;
	const float displacement = (HEIGHT_SCALE * (height - 1));

	// Offset vertex along normal
	posV += output.mNormalV * displacement;
	output.mPosH = mul(float4(posV, 1.0f), gFrameConstants.mP);

	// Compute tangent and binormal
	output.mTangentV = normalize(uvw.x * patch[0].mTangentV + uvw.y * patch[1].mTangentV + uvw.z * patch[2].mTangentV);
	output.mBinormalV = normalize(cross(output.mNormalV, output.mTangentV));
	output.mPosV = posV;

	return output;
}
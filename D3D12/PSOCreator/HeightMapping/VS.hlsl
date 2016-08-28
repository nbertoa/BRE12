#define MIN_TESS_DISTANCE 25.0f
#define MAX_TESS_DISTANCE 1.0f
#define MIN_TESS_FACTOR 1.0f
#define MAX_TESS_FACTOR 5.0f

struct Input {
	float3 mPosO : POSITION;
	float3 mNormalO : NORMAL;
	float3 mTangentO : TANGENT;
	float2 mTexCoordO : TEXCOORD;
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
	float3 mPosV : POSITION;
	float3 mNormalV : NORMAL;
	float3 mTangentV : TANGENT;
	float2 mTexCoordO : TEXCOORD0;
	float mTessFactor : TESS;
};

Output main(in const Input input) {
	const float4x4 wv = mul(gObjConstants.mW, gFrameConstants.mV);

	Output output;
	output.mPosV = mul(float4(input.mPosO, 1.0f), wv).xyz;
	output.mNormalV = mul(input.mNormalO, (float3x3)wv);
	output.mTangentV = mul(input.mTangentO, (float3x3)wv);
	output.mTexCoordO = input.mTexCoordO;

	const float d = length(output.mPosV);

	// Normalized tessellation factor. 
	// The tessellation is 
	//   0 if d >= MIN_TESS_DISTANCE and
	//   1 if d <= MAX_TESS_DISTANCE.  
	const float tess = saturate((MIN_TESS_DISTANCE - d) / (MIN_TESS_DISTANCE - MAX_TESS_DISTANCE));

	// Rescale [0,1] --> [MIN_TESS_FACTOR, MAX_TESS_FACTOR].
	output.mTessFactor = MIN_TESS_FACTOR + tess * (MAX_TESS_FACTOR - MIN_TESS_FACTOR);

	return output;
}
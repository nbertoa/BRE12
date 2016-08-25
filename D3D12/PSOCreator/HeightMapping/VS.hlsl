struct Input {
	float3 mPosO : POSITION;
	float3 mNormalO : NORMAL;
	float3 mTangentO : TANGENT;
	float2 mTexCoordO : TEXCOORD;
};

struct Output {
	float3 mPosO : POSITION;
	float3 mNormalO : NORMAL;
	float3 mTangentO : TANGENT;
	float2 mTexCoordO : TEXCOORD;
};

Output main(in const Input input) {
	Output output;
	output.mPosO = input.mPosO;
	output.mNormalO = input.mNormalO;
	output.mTangentO = input.mTangentO;
	output.mTexCoordO = input.mTexCoordO;

	return output;
}
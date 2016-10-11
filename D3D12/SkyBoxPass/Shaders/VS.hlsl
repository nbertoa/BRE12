#include "../../ShaderUtils/CBuffers.hlsli"

struct Input {
	float3 mPosO : POSITION;
	float3 mNormalO : NORMAL;
	float3 mTangentO : TANGENT;
	float2 mTexCoordO : TEXCOORD;
};

ConstantBuffer<ObjectCBuffer> gObjCBuffer : register(b0);
ConstantBuffer<FrameCBuffer> gFrameCBuffer : register(b1);

struct Output {
	float4 mPosH : SV_POSITION;
	float3 mPosO : POS_VIEW;
};

Output main(in const Input input) {
	const float4x4 vp = mul(gFrameCBuffer.mV, gFrameCBuffer.mP);

	Output output;

	// Use local vertex position as cubemap lookup vector.
	output.mPosO = input.mPosO;

	// Get camera world position.

	// Always center sky about camera.
	float3 posW = mul(float4(input.mPosO, 1.0f), gObjCBuffer.mW).xyz;
	posW += gFrameCBuffer.mEyePosW;

	// Set z = w so that z/w = 1 (i.e., skydome always on far plane).
	output.mPosH = mul(float4(posW, 1.0f), vp).xyww;

	return output;
}
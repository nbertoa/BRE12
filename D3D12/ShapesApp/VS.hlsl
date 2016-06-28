struct Input {
	float3 PosO : POSITION;
	float3 NormalO : NORMAL;
	float3 TangentO : TANGENT;
	float2 TexCoordO : TEXCOORD;
};

struct ObjConstants {
	float4x4 mW;
};
ConstantBuffer<ObjConstants> gObjConstants : register(b0);

struct FrameConstants {
	float4x4 mVP;
};
ConstantBuffer<FrameConstants> gFrameConstants : register(b1);

struct Output {
	
	float4 PosH : SV_POSITION;
};

Output main(in const Input input) {
	Output output;
	float4x4 wvp = mul(gObjConstants.mW, gFrameConstants.mVP);
	output.PosH = mul(float4(input.PosO, 1.0f), wvp);
	return output;
}
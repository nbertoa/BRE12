struct Input {
	float3 PosO : POSITION;
	float3 NormalO : NORMAL;
	float3 TangentO : TANGENT;
	float2 TexCoordO : TEXCOORD;
};

struct ObjConstants {
	float4x4 mWVP;
};
ConstantBuffer<ObjConstants> gObjConstants : register(b0);

struct Output {
	
	float4 PosH : SV_POSITION;
};

Output main(in const Input input) {
	Output output;
	output.PosH = mul(float4(input.PosO, 1.0f), gObjConstants.mWVP);
	return output;
}
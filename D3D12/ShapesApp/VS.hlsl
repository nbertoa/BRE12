struct Input {
	float4 PosO : POSITION;
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
	output.PosH = mul(input.PosO, gObjConstants.mWVP);
	return output;
}
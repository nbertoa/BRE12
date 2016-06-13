#define MyRS \
"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | " \
"DENY_HULL_SHADER_ROOT_ACCESS | " \
"DENY_DOMAIN_SHADER_ROOT_ACCESS | " \
"DENY_GEOMETRY_SHADER_ROOT_ACCESS | " \
"DENY_PIXEL_SHADER_ROOT_ACCESS), " \
"DescriptorTable(CBV(b0))" 

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

[RootSignature(MyRS)]
Output main(in const Input input) {
	Output output;
	output.PosH = mul(input.PosO, gObjConstants.mWVP);
	return output;
}
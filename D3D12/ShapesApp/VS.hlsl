struct Input {
	float4 PosH : POSITION;
	float4 Color : COLOR;
};

struct Output {
	float4 PosH : SV_POSITION;
	float4 Color : COLOR;
};

Output main(in const Input input) {
	Output output;
	output.PosH = input.PosH;
	output.Color = input.Color;
	return output;
}
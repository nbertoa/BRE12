struct Input {
	uint mVertexId : SV_VertexID;
};


struct Output {
	uint mVertexId : DUMMY;
};

Output main(in const Input input) {
	Output output;

	output.mVertexId = input.mVertexId;
	
	return output;
}
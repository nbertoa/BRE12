#ifndef CBUFFERS_HEADER
#define CBUFFERS_HEADER

struct ObjectCBuffer {
	float4x4 mW;
	float mTexTransform;
};

struct FrameCBuffer {
	float4x4 mV;
	float4x4 mP;
	float3 mEyePosW;
};

struct ImmutableCBuffer {
	float4 mNearZ_FarZ_ScreenW_ScreenH;
};

#endif
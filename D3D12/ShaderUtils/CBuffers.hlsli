#ifndef CBUFFERS_HEADER
#define CBUFFERS_HEADER

// Per object constant buffer data
struct ObjectCBuffer {
	float4x4 mW;
	float mTexTransform;
};

// Per frame constant buffer data
struct FrameCBuffer {
	float4x4 mV;
	float4x4 mP;
	float4x4 mInvP;
	float3 mEyePosW;
};

// Immutable constant buffer data (does not change across frames or objects) 
struct ImmutableCBuffer {
	float4 mNearZ_FarZ_ScreenW_ScreenH;
};

#endif
#ifndef CBUFFERS_HEADER
#define CBUFFERS_HEADER

// Per object constant buffer data
struct ObjectCBuffer {
	float4x4 mWorldMatrix;
	float4x4 mInverseTransposeWorldMatrix;
	float mTexTransform;
};

// Per frame constant buffer data
struct FrameCBuffer {	
	float4x4 mViewMatrix;
	float4x4 mInverseViewMatrix;
	float4x4 mProjectionMatrix;
	float4x4 mInverseProjectionMatrix;	
	float4 mEyePositionWorldSpace;
};

// Immutable constant buffer data (does not change across frames or objects) 
struct ImmutableCBuffer {
	float4 mNearZ_FarZ_ScreenW_ScreenH;
};

#endif
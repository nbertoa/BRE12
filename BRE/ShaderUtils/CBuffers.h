#pragma once

#include <DirectXMath.h>

#include <GlobalData\Settings.h>
#include <MathUtils\MathUtils.h>

// Per object constant buffer data
struct ObjectCBuffer {
	ObjectCBuffer() = default;
	~ObjectCBuffer() = default;
	ObjectCBuffer(const ObjectCBuffer&) = default;
	ObjectCBuffer(ObjectCBuffer&&) = default;
	ObjectCBuffer& operator=(ObjectCBuffer&&) = default;

	DirectX::XMFLOAT4X4 mWorld{ MathUtils::Identity4x4() };
	float mTexTransform{ 2.0f };
};

// Per frame constant buffer data
struct FrameCBuffer {
	FrameCBuffer() = default;
	~FrameCBuffer() = default;
	FrameCBuffer(const FrameCBuffer&) = default;
	FrameCBuffer(FrameCBuffer&&) = default;
	FrameCBuffer& operator=(FrameCBuffer&&) = default;
	
	DirectX::XMFLOAT4X4 mView{ MathUtils::Identity4x4() };
	DirectX::XMFLOAT4X4 mInvView{ MathUtils::Identity4x4() };
	DirectX::XMFLOAT4X4 mProj{ MathUtils::Identity4x4() };
	DirectX::XMFLOAT4X4 mInvProj{ MathUtils::Identity4x4() };	
	DirectX::XMFLOAT4 mEyePosW{ 0.0f, 0.0f, 0.0f, 1.0f };
};

// Immutable constant buffer data (does not change across frames or objects) 
struct ImmutableCBuffer {
	ImmutableCBuffer() = default;
	~ImmutableCBuffer() = default;
	ImmutableCBuffer(const ImmutableCBuffer&) = default;
	ImmutableCBuffer(ImmutableCBuffer&&) = default;
	ImmutableCBuffer& operator=(ImmutableCBuffer&&) = default;

	float mNearZ_FarZ_ScreenW_ScreenH[4U] { 
		Settings::sNearPlaneZ,
		Settings::sFarPlaneZ, 
		static_cast<float>(Settings::sWindowWidth), 
		static_cast<float>(Settings::sWindowHeight) 
	};
	
	// Projection constants to get linear depth in view space from 
	// depth stored in depth buffer. These are:
	// Projection A = FarClipDistance / (FarClipDistance - NearClipDistance)
	// Projection B = (-FarClipDistance * NearClipDistance) / (FarClipDistance - NearClipDistance)
	float mProjectionA_ProjectionB[2U]{
		Settings::sFarPlaneZ / (Settings::sFarPlaneZ - Settings::sNearPlaneZ),
		(-Settings::sFarPlaneZ * Settings::sNearPlaneZ) / (Settings::sFarPlaneZ - Settings::sNearPlaneZ)
	};

};

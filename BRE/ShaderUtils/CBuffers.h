#pragma once

#include <DirectXMath.h>

#include <MathUtils\MathUtils.h>
#include <SettingsManager\SettingsManager.h>

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
	const FrameCBuffer& operator=(const FrameCBuffer&);
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
		SettingsManager::sNearPlaneZ,
		SettingsManager::sFarPlaneZ, 
		static_cast<float>(SettingsManager::sWindowWidth), 
		static_cast<float>(SettingsManager::sWindowHeight) 
	};
};

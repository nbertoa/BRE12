#pragma once

#include <DirectXMath.h>

#include <GlobalData\Settings.h>
#include <MathUtils\MathUtils.h>

struct ObjectCBuffer {
	ObjectCBuffer() = default;

	DirectX::XMFLOAT4X4 mWorld{ MathUtils::Identity4x4() };
	float mTexTransform{ 2.0f };
};

struct FrameCBuffer {
	FrameCBuffer() = default;

	DirectX::XMFLOAT4X4 mView{ MathUtils::Identity4x4() };
	DirectX::XMFLOAT4X4 mProj{ MathUtils::Identity4x4() };
	DirectX::XMFLOAT3 mEyePosW{ 0.0f, 0.0f, 0.0f };
};

struct ImmutableCBuffer {
	ImmutableCBuffer() = default;

	float mNearZ_FarZ_ScreenW_ScreenH[4U]{ Settings::sNearPlaneZ, Settings::sFarPlaneZ, (float)Settings::sWindowWidth, (float)Settings::sWindowHeight };
};

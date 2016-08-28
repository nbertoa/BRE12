#pragma once

#include <DirectXMath.h>
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
};

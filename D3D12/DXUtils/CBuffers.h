#pragma once

#include <DirectXMath.h>

struct ObjectCBuffer {
	ObjectCBuffer() = default;

	DirectX::XMFLOAT4X4 mWorld{ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
	float mTexTransform{ 1.0f };
};

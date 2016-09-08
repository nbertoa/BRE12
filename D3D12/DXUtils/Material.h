#pragma once

struct Material {
	Material() = default;

	float mBaseColor_MetalMask[4U]{ 1.0f, 1.0f, 1.0f, 0.0f };
	float mSmoothness{ 1.0f };
	float mPad[3U];
};

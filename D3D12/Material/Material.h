#pragma once

struct Material {
	Material() = default;
	Material(const float c0, const float c1, const float c2, const float metalMask, const float smoothness)
	{
		mBaseColor_MetalMask[0U] = c0;
		mBaseColor_MetalMask[1U] = c1;
		mBaseColor_MetalMask[2U] = c2;
		mBaseColor_MetalMask[3U] = metalMask;
		mSmoothness = smoothness;
	}

	float mBaseColor_MetalMask[4U]{ 1.0f, 1.0f, 1.0f, 0.0f };
	float mSmoothness{ 1.0f };
	float mPad[3U];

	void RandomSmoothness() noexcept;
	void RandomMetalMask() noexcept;
	void RandomBaseColor() noexcept;
	void RandomMaterial() noexcept;
};

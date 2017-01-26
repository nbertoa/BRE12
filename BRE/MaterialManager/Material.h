#pragma once

#include <cstring>

struct Material {
	Material() = default;
	Material(
		const float baseColorR, 
		const float baseColorG, 
		const float baseColorB, 
		const float metalMask, 
		const float smoothness)
	{
		mBaseColor_MetalMask[0U] = baseColorR;
		mBaseColor_MetalMask[1U] = baseColorG;
		mBaseColor_MetalMask[2U] = baseColorB;
		mBaseColor_MetalMask[3U] = metalMask;
		mSmoothness = smoothness;
	}

	~Material() = default;
	Material(const Material&) = default;

	const Material& operator=(const Material& instance) {
		if (this == &instance) {
			return *this;
		}

		memcpy(mBaseColor_MetalMask, instance.mBaseColor_MetalMask, sizeof(mBaseColor_MetalMask));
		mSmoothness = instance.mSmoothness;

		return *this;
	}

	Material(Material&&) = default;
	Material& operator=(Material&&) = default;

	float mBaseColor_MetalMask[4U]{ 1.0f, 1.0f, 1.0f, 0.0f };
	float mSmoothness{ 1.0f };
	float mPadding[3U];

	void RandomizeSmoothness() noexcept;
	void RandomizeMetalMask() noexcept;
	void RandomizeBaseColor() noexcept;
	void RandomizeMaterial() noexcept;
};
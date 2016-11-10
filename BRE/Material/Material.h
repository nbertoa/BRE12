#pragma once

#include <cstring>

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
	float mPad[3U];

	void RandomSmoothness() noexcept;
	void RandomMetalMask() noexcept;
	void RandomBaseColor() noexcept;
	void RandomMaterial() noexcept;
};

namespace Materials {
	enum MaterialType {
		GOLD = 0U,
		SILVER,
		COPPER,
		IRON,
		ALUMINUM,
		PLASTIC_GLASS_LOW,
		PLASTIC_HIGH,
		GLASS_HIGH,
		NUM_MATERIALS
	};

	// You must call this method before GetMaterial()
	void InitMaterials() noexcept;

	const Material& GetMaterial(const MaterialType material) noexcept;
};

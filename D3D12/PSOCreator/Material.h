#pragma once

struct Material {
	Material() = default;
	Material(const Material&) = delete;
	const Material& operator=(const Material&) = delete;

	float mBaseColor_MetalMask[4U]{ 0.0f, 0.0f, 0.0f, 0.0f };
	float mReflectance_Smoothness[4U]{ 0.0f, 0.0f, 0.0f, 0.0f };
};

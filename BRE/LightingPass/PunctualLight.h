#pragma once

struct PunctualLight {
	PunctualLight() = default;
	~PunctualLight() = default;
	PunctualLight(const PunctualLight&) = default;
	PunctualLight(PunctualLight&&) = default;
	PunctualLight& operator=(PunctualLight&&) = default;

	float mPositionAndRange[4U]{ 0.0f, 0.0f, 0.0f, 0.0f };
	float mColorAndPower[4U]{ 0.0f, 0.0f, 0.0f, 0.0f };

	void RandomizePosition(
		const float minX, 
		const float minY, 
		const float minZ,
		const float maxX,
		const float maxY,
		const float maxZ) noexcept;
	void RandomizeRange(const float minRange, const float maxRange) noexcept;
	void RandomizeColor() noexcept;
	void RandomizePower(const float minPower, const float maxPower) noexcept;
};

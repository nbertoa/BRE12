#pragma once

struct PunctualLight {
	PunctualLight() = default;
	~PunctualLight() = default;
	PunctualLight(const PunctualLight&) = default;
	PunctualLight(PunctualLight&&) = default;
	PunctualLight& operator=(PunctualLight&&) = default;

	float mPosAndRange[4U]{ 0.0f, 0.0f, 0.0f, 0.0f };
	float mColorAndPower[4U]{ 0.0f, 0.0f, 0.0f, 0.0f };

	void RandomPosition(
		const float minX, 
		const float minY, 
		const float minZ,
		const float maxX,
		const float maxY,
		const float maxZ) noexcept;
	void RandomRange(const float minRange, const float maxRange) noexcept;
	void RandomColor() noexcept;
	void RandomPower(const float minPower, const float maxPower) noexcept;
};

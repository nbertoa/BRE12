#pragma once

struct PunctualLight {
	PunctualLight() = default;

	float mPosAndRange[4U]{ 0.0f, 0.0f, 0.0f, 0.0f };
	float mColorAndPower[4U]{ 0.0f, 0.0f, 0.0f, 0.0f };
};

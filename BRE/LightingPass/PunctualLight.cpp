#include "PunctualLight.h"

#include <MathUtils/MathUtils.h>
#include <Utils/DebugUtils.h>

void PunctualLight::RandomPosition(
	const float minX,
	const float minY,
	const float minZ,
	const float maxX,
	const float maxY,
	const float maxZ) noexcept
{
	ASSERT(minX < maxX);
	ASSERT(minY < maxY);
	ASSERT(minZ < maxZ);

	mPosAndRange[0U] = MathUtils::RandomFloatInInverval(minX, maxX);
	mPosAndRange[1U] = MathUtils::RandomFloatInInverval(minY, maxY);
	mPosAndRange[2U] = MathUtils::RandomFloatInInverval(minY, maxZ);
}

void PunctualLight::RandomRange(const float minRange, const float maxRange) noexcept {
	ASSERT(minRange < maxRange);

	mPosAndRange[3U] = MathUtils::RandomFloatInInverval(minRange, maxRange);
}

void PunctualLight::RandomColor() noexcept {
	mColorAndPower[0U] = MathUtils::RandomFloatInInverval(0.0f, 1.0f);
	mColorAndPower[1U] = MathUtils::RandomFloatInInverval(0.0f, 1.0f);
	mColorAndPower[2U] = MathUtils::RandomFloatInInverval(0.0f, 1.0f);
}

void PunctualLight::RandomPower(const float minPower, const float maxPower) noexcept {
	ASSERT(minPower < maxPower);

	mColorAndPower[3U] = MathUtils::RandomFloatInInverval(minPower, maxPower);
}
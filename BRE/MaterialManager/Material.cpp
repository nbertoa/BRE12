#include "Material.h"

#include <MathUtils/MathUtils.h>
#include <Utils\DebugUtils.h>

void Material::RandomizeSmoothness() noexcept
{
	mSmoothness = MathUtils::RandomFloatInInverval(0.0f, 1.0f);
}

void Material::RandomizeMetalMask() noexcept
{
	mBaseColor_MetalMask[3U] = static_cast<float>(MathUtils::RandomIntegerInInterval(0U, 1U));
}

void Material::RandomizeBaseColor() noexcept
{
	mBaseColor_MetalMask[0U] = MathUtils::RandomFloatInInverval(0.0f, 1.0f);
	mBaseColor_MetalMask[1U] = MathUtils::RandomFloatInInverval(0.0f, 1.0f);
	mBaseColor_MetalMask[2U] = MathUtils::RandomFloatInInverval(0.0f, 1.0f);
}

void Material::RandomizeMaterial() noexcept
{
	RandomizeSmoothness();
	RandomizeMetalMask();
	RandomizeBaseColor();
}
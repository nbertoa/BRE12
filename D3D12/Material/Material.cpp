#include "Material.h"

#include <MathUtils/MathUtils.h>

void Material::RandomSmoothness() noexcept
{
	mSmoothness = MathUtils::RandF(0.0f, 1.0f);
}

void Material::RandomMetalMask() noexcept
{
	mBaseColor_MetalMask[3U] = static_cast<float>(MathUtils::Rand(0U, 1U));
}

void Material::RandomBaseColor() noexcept
{
	mBaseColor_MetalMask[0U] = MathUtils::RandF(0.0f, 1.0f);
	mBaseColor_MetalMask[1U] = MathUtils::RandF(0.0f, 1.0f);
	mBaseColor_MetalMask[2U] = MathUtils::RandF(0.0f, 1.0f);
}

void Material::RandomMaterial() noexcept
{
	RandomSmoothness();
	RandomMetalMask();
	RandomBaseColor();
}
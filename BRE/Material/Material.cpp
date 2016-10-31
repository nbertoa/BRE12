#include "Material.h"

#include <MathUtils/MathUtils.h>
#include <Utils\DebugUtils.h>

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

namespace {
	Material sMaterials[Materials::NUM_MATERIALS]{};
}

namespace Materials {
	void InitMaterials() noexcept {
		Material* m = &sMaterials[GOLD];
		m->mBaseColor_MetalMask[0U] = 1.0f;
		m->mBaseColor_MetalMask[1U] = 0.71f;
		m->mBaseColor_MetalMask[2U] = 0.29f;
		m->mBaseColor_MetalMask[3U] = 1.0f;
		m->mSmoothness = 0.95f;

		m = &sMaterials[SILVER];
		m->mBaseColor_MetalMask[0U] = 0.95f;
		m->mBaseColor_MetalMask[1U] = 0.93f;
		m->mBaseColor_MetalMask[2U] = 0.88f;
		m->mBaseColor_MetalMask[3U] = 1.0f;
		m->mSmoothness = 0.85f;

		m = &sMaterials[COPPER];
		m->mBaseColor_MetalMask[0U] = 0.95f;
		m->mBaseColor_MetalMask[1U] = 0.64f;
		m->mBaseColor_MetalMask[2U] = 0.54f;
		m->mBaseColor_MetalMask[3U] = 1.0f;
		m->mSmoothness = 0.75f;

		m = &sMaterials[IRON];
		m->mBaseColor_MetalMask[0U] = 0.56f;
		m->mBaseColor_MetalMask[1U] = 0.57f;
		m->mBaseColor_MetalMask[2U] = 0.58f;
		m->mBaseColor_MetalMask[3U] = 1.0f;
		m->mSmoothness = 0.65f;

		m = &sMaterials[ALUMINUM];
		m->mBaseColor_MetalMask[0U] = 0.91f;
		m->mBaseColor_MetalMask[1U] = 0.92f;
		m->mBaseColor_MetalMask[2U] = 0.92f;
		m->mBaseColor_MetalMask[3U] = 1.0f;
		m->mSmoothness = 0.55f;

		m = &sMaterials[PLASTIC_GLASS_LOW];
		m->mBaseColor_MetalMask[0U] = 0.7f;
		m->mBaseColor_MetalMask[1U] = 0.7f;
		m->mBaseColor_MetalMask[2U] = 0.3f;
		m->mBaseColor_MetalMask[3U] = 0.0f;
		m->mSmoothness = 0.98f;

		m = &sMaterials[PLASTIC_HIGH];
		m->mBaseColor_MetalMask[0U] = 0.7f;
		m->mBaseColor_MetalMask[1U] = 0.3f;
		m->mBaseColor_MetalMask[2U] = 0.7f;
		m->mBaseColor_MetalMask[3U] = 0.0f;
		m->mSmoothness = 0.85f;

		m = &sMaterials[GLASS_HIGH];
		m->mBaseColor_MetalMask[0U] = 0.3f;
		m->mBaseColor_MetalMask[1U] = 0.7f;
		m->mBaseColor_MetalMask[2U] = 0.7f;
		m->mBaseColor_MetalMask[3U] = 0.0f;
		m->mSmoothness = 0.75f;
	}

	const Material& GetMaterial(const MaterialType material) noexcept {
		ASSERT(material < NUM_MATERIALS);
		return sMaterials[material];
	}
}
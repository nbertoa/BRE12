#include "MaterialFactory.h"

#include <Utils\DebugUtils.h>

namespace {
	Material sMaterials[MaterialFactory::NUM_MATERIALS]{};
}

void MaterialFactory::InitMaterials() noexcept {
	const float smoothness{ 0.8f };

	Material* m = &sMaterials[GOLD];
	m->mBaseColor_MetalMask[0U] = 1.0f;
	m->mBaseColor_MetalMask[1U] = 0.71f;
	m->mBaseColor_MetalMask[2U] = 0.29f;
	m->mBaseColor_MetalMask[3U] = 1.0f;
	m->mSmoothness = smoothness;

	m = &sMaterials[SILVER];
	m->mBaseColor_MetalMask[0U] = 0.95f;
	m->mBaseColor_MetalMask[1U] = 0.93f;
	m->mBaseColor_MetalMask[2U] = 0.88f;
	m->mBaseColor_MetalMask[3U] = 1.0f;
	m->mSmoothness = smoothness;

	m = &sMaterials[COPPER];
	m->mBaseColor_MetalMask[0U] = 0.95f;
	m->mBaseColor_MetalMask[1U] = 0.64f;
	m->mBaseColor_MetalMask[2U] = 0.54f;
	m->mBaseColor_MetalMask[3U] = 1.0f;
	m->mSmoothness = smoothness;

	m = &sMaterials[IRON];
	m->mBaseColor_MetalMask[0U] = 0.56f;
	m->mBaseColor_MetalMask[1U] = 0.57f;
	m->mBaseColor_MetalMask[2U] = 0.58f;
	m->mBaseColor_MetalMask[3U] = 1.0f;
	m->mSmoothness = smoothness;

	m = &sMaterials[ALUMINUM];
	m->mBaseColor_MetalMask[0U] = 0.91f;
	m->mBaseColor_MetalMask[1U] = 0.92f;
	m->mBaseColor_MetalMask[2U] = 0.92f;
	m->mBaseColor_MetalMask[3U] = 1.0f;
	m->mSmoothness = smoothness;

	m = &sMaterials[PLASTIC_GLASS_LOW];
	m->mBaseColor_MetalMask[0U] = 0.7f;
	m->mBaseColor_MetalMask[1U] = 0.7f;
	m->mBaseColor_MetalMask[2U] = 0.3f;
	m->mBaseColor_MetalMask[3U] = 0.0f;
	m->mSmoothness = smoothness;

	m = &sMaterials[PLASTIC_HIGH];
	m->mBaseColor_MetalMask[0U] = 0.7f;
	m->mBaseColor_MetalMask[1U] = 0.3f;
	m->mBaseColor_MetalMask[2U] = 0.7f;
	m->mBaseColor_MetalMask[3U] = 0.0f;
	m->mSmoothness = smoothness;

	m = &sMaterials[GLASS_HIGH];
	m->mBaseColor_MetalMask[0U] = 0.3f;
	m->mBaseColor_MetalMask[1U] = 0.7f;
	m->mBaseColor_MetalMask[2U] = 0.7f;
	m->mBaseColor_MetalMask[3U] = 0.0f;
	m->mSmoothness = smoothness;
}

const Material& MaterialFactory::GetMaterial(const MaterialType material) noexcept {
	ASSERT(material < NUM_MATERIALS);
	return sMaterials[material];
}
#include "MaterialManager.h"

#include <memory>

#include <Utils\DebugUtils.h>

Material MaterialManager::mMaterials[MaterialManager::NUM_MATERIALS];

void MaterialManager::Init() noexcept {
	Material* material = &mMaterials[GOLD];
	material->mBaseColor_MetalMask[0U] = 1.0f;
	material->mBaseColor_MetalMask[1U] = 0.71f;
	material->mBaseColor_MetalMask[2U] = 0.29f;
	material->mBaseColor_MetalMask[3U] = 1.0f;
	material->mSmoothness = 0.95f;

	material = &mMaterials[SILVER];
	material->mBaseColor_MetalMask[0U] = 0.95f;
	material->mBaseColor_MetalMask[1U] = 0.93f;
	material->mBaseColor_MetalMask[2U] = 0.88f;
	material->mBaseColor_MetalMask[3U] = 1.0f;
	material->mSmoothness = 0.85f;

	material = &mMaterials[COPPER];
	material->mBaseColor_MetalMask[0U] = 0.95f;
	material->mBaseColor_MetalMask[1U] = 0.64f;
	material->mBaseColor_MetalMask[2U] = 0.54f;
	material->mBaseColor_MetalMask[3U] = 1.0f;
	material->mSmoothness = 0.75f;

	material = &mMaterials[IRON];
	material->mBaseColor_MetalMask[0U] = 0.56f;
	material->mBaseColor_MetalMask[1U] = 0.57f;
	material->mBaseColor_MetalMask[2U] = 0.58f;
	material->mBaseColor_MetalMask[3U] = 1.0f;
	material->mSmoothness = 0.65f;

	material = &mMaterials[ALUMINUM];
	material->mBaseColor_MetalMask[0U] = 0.91f;
	material->mBaseColor_MetalMask[1U] = 0.92f;
	material->mBaseColor_MetalMask[2U] = 0.92f;
	material->mBaseColor_MetalMask[3U] = 1.0f;
	material->mSmoothness = 0.55f;

	material = &mMaterials[PLASTIC_GLASS_LOW];
	material->mBaseColor_MetalMask[0U] = 0.7f;
	material->mBaseColor_MetalMask[1U] = 0.7f;
	material->mBaseColor_MetalMask[2U] = 0.3f;
	material->mBaseColor_MetalMask[3U] = 0.0f;
	material->mSmoothness = 0.98f;

	material = &mMaterials[PLASTIC_HIGH];
	material->mBaseColor_MetalMask[0U] = 0.7f;
	material->mBaseColor_MetalMask[1U] = 0.3f;
	material->mBaseColor_MetalMask[2U] = 0.7f;
	material->mBaseColor_MetalMask[3U] = 0.0f;
	material->mSmoothness = 0.85f;

	material = &mMaterials[GLASS_HIGH];
	material->mBaseColor_MetalMask[0U] = 0.3f;
	material->mBaseColor_MetalMask[1U] = 0.7f;
	material->mBaseColor_MetalMask[2U] = 0.7f;
	material->mBaseColor_MetalMask[3U] = 0.0f;
	material->mSmoothness = 0.75f;
}

const Material& MaterialManager::GetMaterial(const MaterialManager::MaterialType material) noexcept {
	ASSERT(material < NUM_MATERIALS);
	return mMaterials[material];
}
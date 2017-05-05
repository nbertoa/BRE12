#include "MaterialTechnique.h"

#include <Utils\DebugUtils.h>

MaterialTechnique::TechniqueType MaterialTechnique::GetType() const noexcept {
	if (mDiffuseTexture != nullptr) {
		if (mNormalTexture != nullptr) {
			if (mHeightTexture != nullptr) {
				return TechniqueType::HEIGHT_MAPPING;
			}
			else {
				return TechniqueType::NORMAL_MAPPING;
			}
		}
		else {
			ASSERT(mHeightTexture == nullptr);
			return TechniqueType::TEXTURE_MAPPING;
		}
	}
	else if (mNormalTexture != nullptr) {
		if (mHeightTexture != nullptr) {
			return TechniqueType::COLOR_HEIGHT_MAPPING;
		}
		else {
			return TechniqueType::COLOR_NORMAL_MAPPING;
		}
	} else {
		ASSERT(mHeightTexture == nullptr);
		return TechniqueType::COLOR_MAPPING;
	}
}

#include "MaterialTechnique.h"

#include <Utils\DebugUtils.h>

namespace BRE {
MaterialTechnique::TechniqueType
MaterialTechnique::GetType() const noexcept
{
    BRE_CHECK_MSG(mDiffuseTexture != nullptr, L"There is no technique without diffuse texture");

    if (mNormalTexture != nullptr) {
        if (mHeightTexture != nullptr) {
            return TechniqueType::HEIGHT_MAPPING;
        } else {
            return TechniqueType::NORMAL_MAPPING;
        }
    } else {
        BRE_CHECK_MSG(mHeightTexture == nullptr, L"There is no technique with diffuse and height texture but no normal texture");
        return TechniqueType::TEXTURE_MAPPING;
    }

}
}
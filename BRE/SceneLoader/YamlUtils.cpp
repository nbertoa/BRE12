#include "YamlUtils.h"

namespace BRE {
bool
YamlUtils::IsDefined(const YAML::Node& node,
                     const char* key)
{
    BRE_ASSERT(key != nullptr);
    YAML::Node attr = node[key];
    return attr.IsDefined();
}
}


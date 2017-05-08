#include "YamlUtils.h"

bool 
YamlUtils::IsDefined(const YAML::Node& node, const char* key) {
	ASSERT(key != nullptr);
	YAML::Node attr = node[key];
	return attr.IsDefined();
}
#pragma once

#include <string>
#include <unordered_map>

#include <SceneLoader/MaterialProperties.h>

namespace YAML {
	class Node;
}

class MaterialPropertiesLoader {
public:
	MaterialPropertiesLoader() {}
	MaterialPropertiesLoader(const MaterialPropertiesLoader&) = delete;
	const MaterialPropertiesLoader& operator=(const MaterialPropertiesLoader&) = delete;
	MaterialPropertiesLoader(MaterialPropertiesLoader&&) = delete;
	MaterialPropertiesLoader& operator=(MaterialPropertiesLoader&&) = delete;

	void LoadMaterialsProperties(const YAML::Node& rootNode) noexcept;

	const MaterialProperties& GetMaterialProperties(const std::string& name) const noexcept;

private:
	std::unordered_map<std::string, MaterialProperties> mMaterialPropertiesByName;
};
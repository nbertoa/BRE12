#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <SceneLoader\DrawableObject.h>
#include <SceneLoader\MaterialTechnique.h>

namespace YAML {
	class Node;
}

class MaterialPropertiesLoader;
class MaterialTechniqueLoader;
class ModelLoader;

class DrawableObjectLoader {
public:
	using DrawableObjectsByModelName = std::unordered_map<std::string, std::vector<DrawableObject>>;

	DrawableObjectLoader(
		const MaterialPropertiesLoader& materialPropertiesLoader,
		const MaterialTechniqueLoader& materialTechniqueLoader,
		const ModelLoader& modelLoader)
		: mMaterialPropertiesLoader(materialPropertiesLoader)
		, mMaterialTechniqueLoader(materialTechniqueLoader)
		, mModelLoader(modelLoader)
	{}

	DrawableObjectLoader(const DrawableObjectLoader&) = delete;
	const DrawableObjectLoader& operator=(const DrawableObjectLoader&) = delete;
	DrawableObjectLoader(DrawableObjectLoader&&) = delete;
	DrawableObjectLoader& operator=(DrawableObjectLoader&&) = delete;

	void LoadDrawableObjects(const YAML::Node& rootNode) noexcept;

	const DrawableObjectsByModelName& GetDrawableObjectsByModelNameByTechniqueType(
		const MaterialTechnique::TechniqueType techniqueType) const noexcept
	{
		return mDrawableObjectsByModelName[techniqueType];
	}

private:
	DrawableObjectsByModelName mDrawableObjectsByModelName[MaterialTechnique::NUM_TECHNIQUES];

	const MaterialPropertiesLoader& mMaterialPropertiesLoader;
	const MaterialTechniqueLoader& mMaterialTechniqueLoader;
	const ModelLoader& mModelLoader;
};
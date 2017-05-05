#pragma once

#include <string>
#include <vector>

#include <SceneLoader\DrawableObject.h>

namespace YAML {
	class Node;
}

class MaterialPropertiesLoader;
class MaterialTechniqueLoader;
class ModelLoader;

class DrawableObjectLoader {
public:
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

	const std::vector<DrawableObject>& GetDrawableObjects() const noexcept { return mDrawableObjects; }

private:
	std::vector<DrawableObject> mDrawableObjects;

	const MaterialPropertiesLoader& mMaterialPropertiesLoader;
	const MaterialTechniqueLoader& mMaterialTechniqueLoader;
	const ModelLoader& mModelLoader;
};
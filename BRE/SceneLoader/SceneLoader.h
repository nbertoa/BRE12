#pragma once

#include <SceneLoader\DrawableObjectLoader.h>
#include <SceneLoader\MaterialPropertiesLoader.h>
#include <SceneLoader\MaterialTechniqueLoader.h>
#include <SceneLoader\ModelLoader.h>
#include <SceneLoader\TextureLoader.h>

struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;

class SceneLoader {
public:
	SceneLoader();
	SceneLoader(const SceneLoader&) = delete;
	const SceneLoader& operator=(const SceneLoader&) = delete;
	SceneLoader(SceneLoader&&) = delete;
	SceneLoader& operator=(SceneLoader&&) = delete;

	void LoadScene(const char* sceneFilePath) noexcept;

private:
	ID3D12CommandAllocator* mCommandAllocator{ nullptr };
	ID3D12GraphicsCommandList* mCommandList{ nullptr };
	ModelLoader mModelLoader;
	TextureLoader mTextureLoader;
	MaterialPropertiesLoader mMaterialPropertiesLoader;
	MaterialTechniqueLoader mMaterialTechniqueLoader;
	DrawableObjectLoader mDrawableObjectLoader;
};
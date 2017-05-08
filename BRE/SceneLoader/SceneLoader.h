#pragma once

#include <memory>

#include <GeometryPass\GeometryPassCmdListRecorder.h>
#include <Scene\Scene.h>
#include <SceneLoader\DrawableObjectLoader.h>
#include <SceneLoader\EnvironmentLoader.h>
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

	Scene* LoadScene(const char* sceneFilePath) noexcept;

private:
	void GenerateGeometryPassRecorders(Scene& scene) noexcept;
	void GenerateGeometryPassRecordersForColorMapping(Scene::GeometryPassCommandListRecorders& commandListRecorders) noexcept;
	void GenerateGeometryPassRecordersForColorNormalMapping(Scene::GeometryPassCommandListRecorders& commandListRecorders) noexcept;
	void GenerateGeometryPassRecordersForColorHeightMapping(Scene::GeometryPassCommandListRecorders& commandListRecorders) noexcept;
	void GenerateGeometryPassRecordersForTextureMapping(Scene::GeometryPassCommandListRecorders& commandListRecorders) noexcept;
	void GenerateGeometryPassRecordersForNormalMapping(Scene::GeometryPassCommandListRecorders& commandListRecorders) noexcept;
	void GenerateGeometryPassRecordersForHeightMapping(Scene::GeometryPassCommandListRecorders& commandListRecorders) noexcept;

	ID3D12CommandAllocator* mCommandAllocator{ nullptr };
	ID3D12GraphicsCommandList* mCommandList{ nullptr };
	ModelLoader mModelLoader;
	TextureLoader mTextureLoader;
	MaterialPropertiesLoader mMaterialPropertiesLoader;
	MaterialTechniqueLoader mMaterialTechniqueLoader;
	DrawableObjectLoader mDrawableObjectLoader;
	EnvironmentLoader mEnvironmentLoader;
};
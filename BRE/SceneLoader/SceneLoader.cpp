#include "SceneLoader.h"

#include <d3d12.h>
#include <string>
#pragma warning( push )
#pragma warning( disable : 4127)
#include <yaml-cpp/yaml.h>
#pragma warning( pop ) 

#include <CommandManager\CommandAllocatorManager.h>
#include <CommandManager\CommandListManager.h>
#include <Utils/DebugUtils.h>

SceneLoader::SceneLoader() 
	: mMaterialTechniqueLoader(mTextureLoader)
	, mDrawableObjectLoader(mMaterialPropertiesLoader, mMaterialTechniqueLoader, mModelLoader)
{

	mCommandAllocator = &CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
	mCommandList = &CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *mCommandAllocator);
	mCommandList->Close();
};

void SceneLoader::LoadScene(const char* sceneFilePath) noexcept {
	ASSERT(sceneFilePath != nullptr);

	const YAML::Node rootNode = YAML::LoadFile(sceneFilePath);
	ASSERT(rootNode.IsDefined());

	mModelLoader.LoadModels(rootNode, *mCommandAllocator, *mCommandList);
	mTextureLoader.LoadTextures(rootNode, *mCommandAllocator, *mCommandList);
	mMaterialPropertiesLoader.LoadMaterialsProperties(rootNode);	
	mMaterialTechniqueLoader.LoadMaterialTechniques(rootNode);
	mDrawableObjectLoader.LoadDrawableObjects(rootNode);
}
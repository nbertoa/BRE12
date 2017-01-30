#include "SceneUtils.h"

#include <d3d12.h>
#include <wrl.h>

#include <CommandListExecutor\CommandListExecutor.h>
#include <DXUtils\DXUtils.h>
#include <ModelManager\ModelManager.h>
#include <ResourceManager\ResourceManager.h>
#include <Utils/DebugUtils.h>

namespace SceneUtils {
	void SceneResources::LoadTextures(
		const std::vector<std::string>& sourceTextureFilenames,
		ID3D12CommandAllocator& cmdAlloc,
		ID3D12GraphicsCommandList& cmdList) noexcept
	{
		const std::size_t numTexturesToLoad = sourceTextureFilenames.size();
		ASSERT(numTexturesToLoad > 0UL);
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> uploadBuffers;
		uploadBuffers.resize(numTexturesToLoad);		

		std::size_t nextTextureAvailableIndex = mTextures.size();
		const std::size_t newTextureCount = mTextures.size() + numTexturesToLoad;
		mTextures.resize(newTextureCount);

		CHECK_HR(cmdList.Reset(&cmdAlloc, nullptr));
		
		for (std::size_t i = 0UL; i < newTextureCount; ++i, ++nextTextureAvailableIndex) {
			ResourceManager::LoadTextureFromFile(
				sourceTextureFilenames[i].c_str(),
				cmdList,
				mTextures[nextTextureAvailableIndex],
				uploadBuffers[i]);
			ASSERT(mTextures[nextTextureAvailableIndex] != nullptr);
		}
		cmdList.Close();

		CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(cmdList);
	}

	ID3D12Resource& SceneResources::GetTexture(const std::size_t index) noexcept {
		ASSERT(index < mTextures.size());
		ID3D12Resource* res = mTextures[index];
		ASSERT(res != nullptr);
		return *res;
	}

	void SceneResources::LoadModels(
		const std::vector<std::string>& modelFiles,
		ID3D12CommandAllocator& cmdAlloc,
		ID3D12GraphicsCommandList& cmdList) noexcept
	{
		ASSERT(mModels.empty());

		const std::size_t numModelsToLoad = modelFiles.size();
		ASSERT(numModelsToLoad > 0UL);

		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> uploadVertexBuffers;
		uploadVertexBuffers.resize(numModelsToLoad);
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> uploadIndexBuffers;
		uploadIndexBuffers.resize(numModelsToLoad);

		std::size_t nextModelAvailableIndex = mModels.size();
		const std::size_t newModelCount = mModels.size() + numModelsToLoad;
		mModels.resize(newModelCount);

		CHECK_HR(cmdList.Reset(&cmdAlloc, nullptr));

		for (std::size_t i = 0UL; i < numModelsToLoad; ++i, ++nextModelAvailableIndex) {
			ModelManager::LoadModel(
				modelFiles[i].c_str(), 
				mModels[nextModelAvailableIndex], 
				cmdList, 
				uploadVertexBuffers[i], 
				uploadIndexBuffers[i]);
			ASSERT(mTextures[nextModelAvailableIndex] != nullptr);
		}
		cmdList.Close();

		CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(cmdList);
	}

	const Model& SceneResources::GetModel(const std::size_t index) const noexcept {
		ASSERT(index < mModels.size());
		Model* model = mModels[index];
		ASSERT(model != nullptr);
		return *model;
	}
}


#include "SceneUtils.h"

#include <d3d12.h>
#include <wrl.h>

#include <ModelManager\ModelManager.h>
#include <ResourceManager\ResourceManager.h>
#include <Utils/DebugUtils.h>

namespace {
	void ExecuteCommandList(
		ID3D12CommandQueue& cmdQueue, 
		ID3D12GraphicsCommandList& cmdList, 
		ID3D12Fence& fence) noexcept {

		cmdList.Close();

		ID3D12CommandList* cmdLists[1U]{ &cmdList };
		cmdQueue.ExecuteCommandLists(_countof(cmdLists), cmdLists);

		const std::uint64_t fenceValue = fence.GetCompletedValue() + 1UL;

		CHECK_HR(cmdQueue.Signal(&fence, fenceValue));

		// Wait until the GPU has completed commands up to this fence point.
		if (fence.GetCompletedValue() < fenceValue) {
			const HANDLE eventHandle{ CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS) };
			ASSERT(eventHandle);

			// Fire event when GPU hits current fence.  
			CHECK_HR(fence.SetEventOnCompletion(fenceValue, eventHandle));

			// Wait until the GPU hits current fence event is fired.
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}
}

namespace SceneUtils {
	void ResourceContainer::LoadTextures(
		const std::vector<std::string>& texFiles,
		ID3D12CommandQueue& cmdQueue,
		ID3D12CommandAllocator& cmdAlloc,
		ID3D12GraphicsCommandList& cmdList,
		ID3D12Fence& fence) noexcept
	{
		ASSERT(mTextures.empty());

		const std::size_t texCount = texFiles.size();
		ASSERT(texCount > 0UL);

		CHECK_HR(cmdList.Reset(&cmdAlloc, nullptr));

		mTextures.resize(texCount);

		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> uploadBuffers;
		uploadBuffers.resize(texCount);
		
		for (std::size_t i = 0UL; i < texCount; ++i) {
			ResourceManager::Get().LoadTextureFromFile(texFiles[i].c_str(), mTextures[i], uploadBuffers[i], cmdList);
			ASSERT(mTextures[i] != nullptr);
		}

		ExecuteCommandList(cmdQueue, cmdList, fence);
	}

	ID3D12Resource& ResourceContainer::GetResource(const std::size_t index) noexcept {
		ASSERT(index < mTextures.size());
		ID3D12Resource* res = mTextures[index];
		ASSERT(res != nullptr);
		return *res;
	}

	void ResourceContainer::LoadModels(
		const std::vector<std::string>& modelFiles,
		ID3D12CommandQueue& cmdQueue,
		ID3D12CommandAllocator& cmdAlloc,
		ID3D12GraphicsCommandList& cmdList,
		ID3D12Fence& fence) noexcept
	{
		ASSERT(mModels.empty());

		const std::size_t modelCount = modelFiles.size();
		ASSERT(modelCount > 0UL);

		CHECK_HR(cmdList.Reset(&cmdAlloc, nullptr));

		mModels.resize(modelCount);

		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> uploadVertexBuffers;
		uploadVertexBuffers.resize(modelCount);
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> uploadIndexBuffers;
		uploadIndexBuffers.resize(modelCount);

		for (std::size_t i = 0UL; i < modelCount; ++i) {
			ModelManager::Get().LoadModel(modelFiles[i].c_str(), mModels[i], cmdList, uploadVertexBuffers[i], uploadIndexBuffers[i]);
			ASSERT(mTextures[i] != nullptr);
		}

		ExecuteCommandList(cmdQueue, cmdList, fence);
	}

	Model& ResourceContainer::GetModel(const std::size_t index) noexcept {
		ASSERT(index < mModels.size());
		Model* model = mModels[index];
		ASSERT(model != nullptr);
		return *model;
	}
}


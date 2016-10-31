#include "SceneUtils.h"

#include <d3d12.h>
#include <wrl.h>

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
	void LoadTextures(
		const std::vector<std::string>& texFiles,
		ID3D12CommandQueue& cmdQueue,
		ID3D12CommandAllocator& cmdAlloc,
		ID3D12GraphicsCommandList& cmdList,
		ID3D12Fence& fence,
		std::vector<ID3D12Resource*>& textures) noexcept
	{
		const std::size_t texCount = texFiles.size();
		ASSERT(texCount > 0UL);

		CHECK_HR(cmdList.Reset(&cmdAlloc, nullptr));

		textures.resize(texCount);

		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> uploadBuffers;
		uploadBuffers.resize(texCount);
		
		for (std::size_t i = 0UL; i < texCount; ++i) {
			ResourceManager::Get().LoadTextureFromFile(texFiles[i].c_str(), textures[i], uploadBuffers[i], cmdList);
			ASSERT(textures[0] != nullptr);
		}

		ExecuteCommandList(cmdQueue, cmdList, fence);
	}
}


#include "DXUtils.h"

#include <cstdint>
#include <d3d12.h>

#include <Utils\DebugUtils.h>

namespace DXUtils {
	void ExecuteCommandListAndWaitForCompletion(
		ID3D12CommandQueue& cmdQueue,
		ID3D12GraphicsCommandList& cmdList,
		ID3D12Fence& fence) noexcept
	{
		cmdList.Close();

		ID3D12CommandList* cmdLists[1U]{ &cmdList };
		cmdQueue.ExecuteCommandLists(_countof(cmdLists), cmdLists);

		const std::uint64_t completedFenceValue = fence.GetCompletedValue();
		const std::uint64_t newFenceValue = completedFenceValue + 1UL;

		CHECK_HR(cmdQueue.Signal(&fence, newFenceValue));

		// Wait until the GPU has completed commands up to this fence point.
		if (completedFenceValue < newFenceValue) {
			const HANDLE eventHandle{ CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS) };
			ASSERT(eventHandle);

			CHECK_HR(fence.SetEventOnCompletion(newFenceValue, eventHandle));

			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}
}
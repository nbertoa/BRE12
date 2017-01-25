#include "DXUtils.h"

#include <cstdint>
#include <d3d12.h>

#include <Utils\DebugUtils.h>

namespace DXUtils {
	void ExecuteCommandListAndWaitForCompletion(CommandListData& cmdListData) noexcept
	{
		cmdListData.mCmdList.Close();

		ID3D12CommandList* cmdLists[1U]{ &cmdListData.mCmdList };
		cmdListData.mCmdQueue.ExecuteCommandLists(_countof(cmdLists), cmdLists);

		const std::uint64_t completedFenceValue = cmdListData.mFence.GetCompletedValue();
		const std::uint64_t newFenceValue = completedFenceValue + 1UL;

		CHECK_HR(cmdListData.mCmdQueue.Signal(&cmdListData.mFence, newFenceValue));

		// Wait until the GPU has completed commands up to this fence point.
		if (completedFenceValue < newFenceValue) {
			const HANDLE eventHandle{ CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS) };
			ASSERT(eventHandle);

			CHECK_HR(cmdListData.mFence.SetEventOnCompletion(newFenceValue, eventHandle));

			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}
}
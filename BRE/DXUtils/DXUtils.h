#pragma once

struct ID3D12CommandQueue;
struct ID3D12Fence;
struct ID3D12GraphicsCommandList;

namespace DXUtils {
	struct CommandListData {
		CommandListData(
			ID3D12CommandQueue& cmdQueue,
			ID3D12GraphicsCommandList& cmdList,
			ID3D12Fence& fence)
			: mCmdQueue(cmdQueue)
			, mCmdList(cmdList)
			, mFence(fence)
		{}

		~CommandListData() = default;
		CommandListData(const CommandListData&) = delete;
		const CommandListData& operator=(const CommandListData&) = delete;
		CommandListData(CommandListData&&) = delete;
		CommandListData& operator=(CommandListData&&) = delete;

		ID3D12CommandQueue& mCmdQueue;
		ID3D12GraphicsCommandList& mCmdList;
		ID3D12Fence& mFence;
	};

	// Preconditions:
	// - Command list must not be closed
	void ExecuteCommandListAndWaitForCompletion(CommandListData& cmdListData) noexcept;
}

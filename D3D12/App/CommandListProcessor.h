#pragma once

#include <atomic>
#include <d3d12.h>
#include <tbb/concurrent_queue.h>
#include <tbb/task.h>
#include <wrl.h>

// You should use the static method to span a new command list processor thread.
// Execute methods will check for command lists in its queue and execute them in cmdQueue.
// It will execute at most mMaxNumCmdLists per execution operation.
class CommandListProcessor : public tbb::task {
public:
	static tbb::empty_task* Create(CommandListProcessor* &cmdListProcessor, ID3D12CommandQueue* cmdQueue, const std::uint32_t maxNumCmdLists);

	CommandListProcessor(ID3D12CommandQueue* cmdQueue, const std::uint32_t maxNumCmdLists);

	__forceinline bool IsIdle() const noexcept { return mCmdListQueue.empty() && mPendingCmdLists == 0; }
	__forceinline void resetExecutedTasksCounter() noexcept { mExecTasksCount = 0U; }
	__forceinline std::uint32_t ExecutedTasksCounter() const noexcept { return mExecTasksCount; }
	
	tbb::task* execute() override;

	__forceinline tbb::concurrent_queue<ID3D12CommandList*>& CmdListQueue() noexcept { return mCmdListQueue; }

private:
	std::uint32_t mExecTasksCount{ 0U };
	std::atomic<std::uint32_t> mPendingCmdLists{ 0U };
	std::uint32_t mMaxNumCmdLists{ 1U };
	ID3D12CommandQueue* mCmdQueue{ nullptr };
	tbb::concurrent_queue<ID3D12CommandList*> mCmdListQueue;
};

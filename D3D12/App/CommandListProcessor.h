#pragma once

#include <atomic>
#include <d3d12.h>
#include <tbb/concurrent_queue.h>
#include <tbb/task.h>
#include <wrl.h>

// It has the responsibility to check for new command lists and execute them.
// Steps:
// - Use CommandListProcessor::Create() to create an instance. You should
//   spawn it using the returned parent tbb::task.
// - When you spawn it, execute() method is automatically called. You should fill queue with
//   command lists. You can use CommandListProcessor::CmdListQueue() for it.
// - When you want to terminate this task, you should call CommandListProcessor::Terminate() 
//   and wait for termination using parent task.
class CommandListProcessor : public tbb::task {
public:
	static tbb::empty_task* Create(CommandListProcessor* &cmdListProcessor, ID3D12CommandQueue* cmdQueue, const std::uint32_t maxNumCmdLists);

	CommandListProcessor(ID3D12CommandQueue* cmdQueue, const std::uint32_t maxNumCmdLists);

	// This method is used to know if there are no more pending commands lists to execute or to process.
	// I thought this method was going to be thread safe, but that is not the case.
	// As I did not discover yet, why it is not thread safe, then I only use it for debugging purposes.
	__forceinline bool IsIdle() const noexcept { return mCmdListQueue.empty() && mPendingCmdLists == 0; }

	// A thread safe way to know if CommandListProcessor finished processing and executing all the command lists.
	// If you are going to execute N command lists, then you should:
	// - Call ResetExecutedTasksCounter()
	// - Fill queue through CmdListQueue()
	// - Check if ExecutedTasksCounter() is equal to N, to be sure all was executed properly (sent to GPU)
	__forceinline void ResetExecutedTasksCounter() noexcept { mExecTasksCount = 0U; }
	__forceinline std::uint32_t ExecutedTasksCounter() const noexcept { return mExecTasksCount; }
	
	tbb::task* execute() override;
	void Terminate() noexcept { mTerminate = true; }

	__forceinline tbb::concurrent_queue<ID3D12CommandList*>& CmdListQueue() noexcept { return mCmdListQueue; }

private:
	bool mTerminate{ false };
	std::uint32_t mExecTasksCount{ 0U };
	std::atomic<std::uint32_t> mPendingCmdLists{ 0U };
	std::uint32_t mMaxNumCmdLists{ 1U };
	ID3D12CommandQueue* mCmdQueue{ nullptr };
	tbb::concurrent_queue<ID3D12CommandList*> mCmdListQueue;
};

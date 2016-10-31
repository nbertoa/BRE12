#pragma once

#include <atomic>
#include <d3d12.h>
#include <tbb/concurrent_queue.h>
#include <tbb/task.h>

// It has the responsibility to constantly check for new command lists and execute them.
// Steps:
// - Use CommandListExecutor::Create() to create and spawn an instance.
// - When you spawn it, execute() method is automatically called. You should fill the queue with
//   command lists. You can use CommandListExecutor::CmdListQueue() fto get it.
// - When you want to terminate this task, you should call CommandListExecutor::Terminate() 
class CommandListExecutor : public tbb::task {
public:
	// maxNumCmdLists is the maximum number of command lists to execute 
	// by ID3D12CommandQueue::ExecuteCommandLists() operation.
	static CommandListExecutor* Create(ID3D12CommandQueue* cmdQueue, const std::uint32_t maxNumCmdLists) noexcept;

	// This method is used to know if there are no more pending commands lists to execute or to process.
	// I thought this method was going to be thread safe, but that is not the case.
	// As I did not discover yet, why it is not thread safe, then I only use it for debugging purposes.
	__forceinline bool IsIdle() const noexcept { return mCmdListQueue.empty() && mPendingCmdLists == 0; }

	// A thread safe way to know if CommandListExecutor finished processing and executing all the command lists.
	// If you are going to execute N command lists, then you should:
	// - Call ResetExecutedCmdListCount()
	// - Fill queue through CmdListQueue()
	// - Check if ExecutedCmdListCount() is equal to N, to be sure all was executed properly (sent to GPU)
	__forceinline void ResetExecutedCmdListCount() noexcept { mExecutedCmdLists = 0U; }
	__forceinline std::uint32_t ExecutedCmdListCount() const noexcept { return mExecutedCmdLists; }
	
	// You should push all your recorded command lists in this queue
	__forceinline tbb::concurrent_queue<ID3D12CommandList*>& CmdListQueue() noexcept { return mCmdListQueue; }
		
	void Terminate() noexcept;	

private:
	explicit CommandListExecutor(ID3D12CommandQueue* cmdQueue, const std::uint32_t maxNumCmdLists);

	// Called when tbb::task is spawned
	tbb::task* execute() override;

	bool mTerminate{ false };
	std::uint32_t mExecutedCmdLists{ 0U };
	std::atomic<std::uint32_t> mPendingCmdLists{ 0U };
	std::uint32_t mMaxNumCmdLists{ 1U };
	ID3D12CommandQueue* mCmdQueue{ nullptr };
	tbb::concurrent_queue<ID3D12CommandList*> mCmdListQueue;
};

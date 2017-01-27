#pragma once

#include <atomic>
#include <d3d12.h>
#include <tbb/concurrent_queue.h>
#include <tbb/task.h>

// To check for new command lists and execute them.
// Steps:
// - Use CommandListExecutor::Create() to create and spawn an instance.
// - When you spawn it, execute() method is automatically called. You should fill the queue with
//   command lists. You can use CommandListExecutor::GetCommandListQueue() to get it.
// - When you want to terminate this task, you should call CommandListExecutor::Terminate() 
class CommandListExecutor : public tbb::task {
public:
	// maxNumberOfCommandListsToExecute is the maximum number of command lists to execute 
	// by ID3D12CommandQueue::ExecuteCommandLists() operation.
	// Preconditions:
	// - Create() must be called once
	// - "maxNumberOfCommandListsToExecute" must be greater than zero
	static CommandListExecutor& Create(
		ID3D12CommandQueue& commandQueue, 
		const std::uint32_t maxNumberOfCommandListsToExecute) noexcept;

	// Preconditions:
	// - Create() must be called before this method
	static CommandListExecutor& CommandListExecutor::Get() noexcept;
	
	~CommandListExecutor() = default;
	CommandListExecutor(const CommandListExecutor&) = delete;
	const CommandListExecutor& operator=(const CommandListExecutor&) = delete;
	CommandListExecutor(CommandListExecutor&&) = delete;
	CommandListExecutor& operator=(CommandListExecutor&&) = delete;

	// As I did not discover yet, why it is not thread safe, then I only use it for debugging purposes.
	__forceinline bool AreTherePendingCommandListsToExecute() const noexcept { 
		return mCommandListsToExecute.empty() && mPendingCommandListCount == 0; 
	}

	// A thread safe way to know if CommandListExecutor finished processing and executing all the command lists.
	// If you are going to execute N command lists, then you should:
	// - Call ResetExecutedCommandListCount()
	// - Fill queue through GetCommandListQueue()
	// - Check if GetExecutedCommandListCount() is equal to N, to be sure all was executed properly (sent to GPU)
	__forceinline void ResetExecutedCommandListCount() noexcept { mExecutedCommandListCount = 0U; }
	__forceinline std::uint32_t GetExecutedCommandListCount() const noexcept { return mExecutedCommandListCount; }
	
	__forceinline void AddCommandList(ID3D12CommandList& commandList) noexcept { mCommandListsToExecute.push(&commandList); }
		
	void Terminate() noexcept;	

private:
	explicit CommandListExecutor(ID3D12CommandQueue& cmdQueue, const std::uint32_t maxNumCmdLists);

	// Called when tbb::task is spawned
	tbb::task* execute() final override;

	bool mTerminate{ false };
	std::uint32_t mExecutedCommandListCount{ 0U };
	std::atomic<std::uint32_t> mPendingCommandListCount{ 0U };
	std::uint32_t mMaxNumberOfCommandListsToExecute{ 1U };
	ID3D12CommandQueue* mCommandQueue{ nullptr };
	tbb::concurrent_queue<ID3D12CommandList*> mCommandListsToExecute;
};

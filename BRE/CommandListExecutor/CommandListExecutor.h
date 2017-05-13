#pragma once

#include <atomic>
#include <d3d12.h>
#include <tbb/concurrent_queue.h>
#include <tbb/task.h>

#include <Utils\DebugUtils.h>

namespace BRE {

///
/// @brief Class responsible to execute command lists.
///
/// To check for new command lists and execute them.
/// Steps:
/// - Use CommandListExecutor::Create() to create and spawn an instance.
/// - When you spawn it, execute() method is automatically called. You should fill the queue with
///   command lists. You can use CommandListExecutor::GetCommandListQueue() to get it.
/// - When you want to terminate this task, you should call CommandListExecutor::Terminate() 
class CommandListExecutor : public tbb::task {
public:
    ///
    /// @brief Create an instance of CommandListExecutor. 
    ///
    /// This method must be called once.
    ///
    /// @param maxNumberOfCommandListsToExecute The maximum number of command lists to
    /// execute by ID3D12CommandQueue::ExecuteCommandLists(). This parameter must be
    /// greater than zero.
    ///
    static void Create(const std::uint32_t maxNumberOfCommandListsToExecute) noexcept;

    ///
    /// @brief Get CommandListExecutor. 
    ///
    /// CommandListExecutor::Create() must be called before this method.
    ///
    /// @return CommandListExecutor generated with Create() method
    ///
    static CommandListExecutor& CommandListExecutor::Get() noexcept;

    ~CommandListExecutor() = default;
    CommandListExecutor(const CommandListExecutor&) = delete;
    const CommandListExecutor& operator=(const CommandListExecutor&) = delete;
    CommandListExecutor(CommandListExecutor&&) = delete;
    CommandListExecutor& operator=(CommandListExecutor&&) = delete;
    ///
    /// @brief Reset the counter of executed command lists.
    ///
    /// A thread safe way to know if CommandListExecutor finished processing and executing all the command lists.
    /// If you are going to execute N command lists, then you should:
    /// - Call ResetExecutedCommandListCount()
    /// - Fill queue through GetCommandListQueue()
    /// - Check if GetExecutedCommandListCount() is equal to N, to be sure all was executed properly (sent to GPU)
    ///
    __forceinline void ResetExecutedCommandListCount() noexcept
    {
        mExecutedCommandListCount = 0U;
    }

    ///
    /// @brief Get the number of executed command lists.
    ///
    /// @return The number of executed command lists
    ///
    __forceinline std::uint32_t GetExecutedCommandListCount() const noexcept
    {
        return mExecutedCommandListCount;
    }

    ///
    /// @brief Add a command list to be executed
    ///
    /// @param commandList The command list to add
    ///
    __forceinline void AddCommandList(ID3D12CommandList& commandList) noexcept
    {
        mCommandListsToExecute.push(&commandList);
    }

    ///
    /// @brief Get the command queue
    ///
    /// @return The command queue
    ///
    __forceinline ID3D12CommandQueue& GetCommandQueue() noexcept
    {
        BRE_ASSERT(mCommandQueue != nullptr);
        return *mCommandQueue;
    }

    ///
    /// @brief Signal a fence and wait until fence completes.
    ///
    /// @param fence The fence to be updated
    /// @param valueToSignal The value used to update @p fence
    /// @param valueToWaitFor The value that @p fence must reach before we 
    /// return from this method. Commonly, this will be equal to @p valueToSignal.
    ///
    void SignalFenceAndWaitForCompletion(ID3D12Fence& fence,
                                         const std::uint64_t valueToSignal,
                                         const std::uint64_t valueToWaitFor) noexcept;

    ///
    /// @brief Executes a command list and wait until it completes
    ///
    /// @param commandList The command list to be executed
    void ExecuteCommandListAndWaitForCompletion(ID3D12CommandList& commandList) noexcept;

    ///
    /// @brief Terminates the generated CommandListExecutor.
    ///
    void Terminate() noexcept;

private:
    explicit CommandListExecutor(const std::uint32_t maxNumCmdLists);

    // Called when tbb::task is spawned
    tbb::task* execute() final override;

    static CommandListExecutor* sExecutor;

    bool mTerminate{ false };

    std::uint32_t mExecutedCommandListCount{ 0U };
    std::atomic<std::uint32_t> mPendingCommandListCount{ 0U };
    std::uint32_t mMaxNumberOfCommandListsToExecute{ 1U };

    ID3D12CommandQueue* mCommandQueue{ nullptr };
    tbb::concurrent_queue<ID3D12CommandList*> mCommandListsToExecute;
    ID3D12Fence* mFence{ nullptr };
};

}
#pragma once

#include <memory>
#include <tbb/concurrent_queue.h>

#include <SettingsManager\SettingsManager.h>
#include <ToneMappingPass\ToneMappingCmdListRecorder.h>

class CommandListExecutor;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct ID3D12CommandAllocator;
struct ID3D12CommandList;
struct ID3D12CommandQueue;
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;

// Pass that applies tone mapping
class ToneMappingPass {
public:
	using Recorder = std::unique_ptr<ToneMappingCmdListRecorder>;

	ToneMappingPass() = default;
	~ToneMappingPass() = default; 
	ToneMappingPass(const ToneMappingPass&) = delete;
	const ToneMappingPass& operator=(const ToneMappingPass&) = delete;
	ToneMappingPass(ToneMappingPass&&) = delete;
	ToneMappingPass& operator=(ToneMappingPass&&) = delete;

	// You should call this method before Execute()
	void Init(
		CommandListExecutor& cmdListExecutor,
		ID3D12CommandQueue& cmdQueue,
		ID3D12Resource& inputColorBuffer,
		ID3D12Resource& outputColorBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& outputBufferCpuDesc) noexcept;

	void Execute() noexcept;

private:
	// Method used internally for validation purposes
	bool ValidateData() const noexcept;

	void ExecuteBeginTask() noexcept;

	CommandListExecutor* mCmdListExecutor{ nullptr };
	ID3D12CommandQueue* mCmdQueue{ nullptr };

	// 1 command allocater per queued frame.	
	ID3D12CommandAllocator* mCmdAllocs[SettingsManager::sQueuedFrameCount]{ nullptr };

	ID3D12GraphicsCommandList* mCmdList{ nullptr };

	ID3D12Fence* mFence{ nullptr };
	
	ID3D12Resource* mInputColorBuffer{ nullptr };
	ID3D12Resource* mOutputColorBuffer{ nullptr };

	Recorder mRecorder;
};

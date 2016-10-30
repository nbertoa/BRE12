#pragma once

#include <memory>
#include <tbb/concurrent_queue.h>

#include <GlobalData\Settings.h>
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
	ToneMappingPass(const ToneMappingPass&) = delete;
	const ToneMappingPass& operator=(const ToneMappingPass&) = delete;

	// You should call this method before Execute()
	void Init(
		ID3D12Device& device,
		CommandListExecutor& cmdListProcessor,
		ID3D12CommandQueue& cmdQueue,
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		ID3D12Resource& colorBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept;

	void Execute(
		ID3D12Resource& frameBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& frameBufferCpuDesc) noexcept;

private:
	// Method used internally for validation purposes
	bool ValidateData() const noexcept;

	void ExecuteBeginTask(
		ID3D12Resource& frameBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& frameBufferCpuDesc) noexcept;

	CommandListExecutor* mCmdListProcessor{ nullptr };
	ID3D12CommandQueue* mCmdQueue{ nullptr };

	// 1 command allocater per queued frame.	
	ID3D12CommandAllocator* mCmdAllocs[Settings::sQueuedFrameCount]{ nullptr };

	ID3D12GraphicsCommandList* mCmdList{ nullptr };

	ID3D12Fence* mFence{ nullptr };
	
	ID3D12Resource* mColorBuffer{ nullptr };

	Recorder mRecorder;
};

#pragma once

#include <d3d12.h>
#include <tbb/concurrent_queue.h>

#include <GlobalData/Settings.h>

class UploadBuffer;

// Responsible of command lists recording to be executed by CommandListExecutor.
// This class has common data and functionality to record command list for tone mapping pass.
class ToneMappingCmdListRecorder {
public:
	explicit ToneMappingCmdListRecorder(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);
	~ToneMappingCmdListRecorder() = default;
	ToneMappingCmdListRecorder(const ToneMappingCmdListRecorder&) = delete;
	const ToneMappingCmdListRecorder& operator=(const ToneMappingCmdListRecorder&) = delete;
	ToneMappingCmdListRecorder(ToneMappingCmdListRecorder&&) = default;
	ToneMappingCmdListRecorder& operator=(ToneMappingCmdListRecorder&&) = default;

	// This method is to initialize PSO that is a shared between all this kind
	// of recorders.
	// This method is initialized by its corresponding pass.
	static void InitPSO() noexcept;

	void Init(
		ID3D12Resource& inputColorBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& outputBufferCpuDesc) noexcept;

	void RecordAndPushCommandLists() noexcept;

	bool ValidateData() const noexcept;

private:
	void BuildBuffers(ID3D12Resource& colorBuffer) noexcept;

	tbb::concurrent_queue<ID3D12CommandList*>& mCmdListQueue;

	ID3D12GraphicsCommandList* mCmdList{ nullptr };
	ID3D12CommandAllocator* mCmdAlloc[Settings::sQueuedFrameCount]{ nullptr };
		
	D3D12_GPU_DESCRIPTOR_HANDLE mInputColorBufferGpuDesc{ 0UL };
	D3D12_CPU_DESCRIPTOR_HANDLE mOutputColorBufferCpuDesc{ 0UL };
};
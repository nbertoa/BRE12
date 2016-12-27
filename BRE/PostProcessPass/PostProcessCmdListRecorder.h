#pragma once

#include <d3d12.h>
#include <tbb/concurrent_queue.h>

#include <GlobalData/Settings.h>

class UploadBuffer;

// Responsible of command lists recording to be executed by CommandListExecutor.
// This class has common data and functionality to record command list for post processing effects (anti aliasing, color grading, etc).
class PostProcessCmdListRecorder {
public:
	explicit PostProcessCmdListRecorder(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);
	~PostProcessCmdListRecorder() = default;
	PostProcessCmdListRecorder(const PostProcessCmdListRecorder&) = delete;
	const PostProcessCmdListRecorder& operator=(const PostProcessCmdListRecorder&) = delete;
	PostProcessCmdListRecorder(PostProcessCmdListRecorder&&) = default;
	PostProcessCmdListRecorder& operator=(PostProcessCmdListRecorder&&) = default;

	// This method is to initialize PSO that is a shared between all this kind
	// of recorders.
	// This method is initialized by its corresponding pass.
	static void InitPSO() noexcept;

	void Init(ID3D12Resource& colorBuffer) noexcept;

	void RecordAndPushCommandLists(const D3D12_CPU_DESCRIPTOR_HANDLE& frameBufferCpuDesc) noexcept;

	bool ValidateData() const noexcept;

private:
	void BuildBuffers(ID3D12Resource& colorBuffer) noexcept;

	tbb::concurrent_queue<ID3D12CommandList*>& mCmdListQueue;

	ID3D12GraphicsCommandList* mCmdList{ nullptr };
	ID3D12CommandAllocator* mCmdAlloc[Settings::sQueuedFrameCount]{ nullptr };

	D3D12_GPU_DESCRIPTOR_HANDLE mColorBufferGpuDesc{ 0UL };
};
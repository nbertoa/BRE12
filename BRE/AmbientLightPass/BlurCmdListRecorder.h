#pragma once

#include <d3d12.h>
#include <tbb/concurrent_queue.h>

#include <GlobalData/Settings.h>

class UploadBuffer;

// Responsible of command lists recording to be executed by CommandListExecutor.
// This class has common data and functionality to record command list to blur a color buffer.
class BlurCmdListRecorder {
public:
	explicit BlurCmdListRecorder(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);
	~BlurCmdListRecorder() = default;
	BlurCmdListRecorder(const BlurCmdListRecorder&) = delete;
	const BlurCmdListRecorder& operator=(const BlurCmdListRecorder&) = delete;
	BlurCmdListRecorder(BlurCmdListRecorder&&) = default;
	BlurCmdListRecorder& operator=(BlurCmdListRecorder&&) = default;

	// This method is to initialize PSO that is a shared between all this kind
	// of recorders.
	// This method is initialized by its corresponding pass.
	static void InitPSO() noexcept;

	void Init(
		ID3D12Resource& ambientAccessibilityBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& blurBufferCpuDesc) noexcept;

	void RecordAndPushCommandLists() noexcept;

	bool ValidateData() const noexcept;

private:
	void BuildBuffers(ID3D12Resource& colorBuffer) noexcept;

	tbb::concurrent_queue<ID3D12CommandList*>& mCmdListQueue;

	ID3D12GraphicsCommandList* mCmdList{ nullptr };
	ID3D12CommandAllocator* mCmdAlloc[Settings::sQueuedFrameCount]{ nullptr };
	std::uint32_t mCurrFrameIndex{ 0U };
	
	D3D12_GPU_DESCRIPTOR_HANDLE mColorBufferGpuDescHandle{ 0UL };

	D3D12_CPU_DESCRIPTOR_HANDLE mBlurBufferCpuDescHandle{ 0UL };
};
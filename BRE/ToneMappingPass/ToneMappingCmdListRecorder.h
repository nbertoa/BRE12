#pragma once

#include <d3d12.h>
#include <tbb/concurrent_queue.h>

#include <GlobalData/Settings.h>

class UploadBuffer;

// Responsible of command lists recording to be executed by CommandListExecutor.
// This class has common data and functionality to record command list for tone mapping pass.
class ToneMappingCmdListRecorder {
public:
	explicit ToneMappingCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);
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
		ID3D12Resource& colorBuffer,
		ID3D12Resource& depthBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept;

	void RecordAndPushCommandLists(const D3D12_CPU_DESCRIPTOR_HANDLE& frameBufferCpuDesc) noexcept;

	bool ValidateData() const noexcept;

private:
	void BuildBuffers(ID3D12Resource& colorBuffer, ID3D12Resource& depthBuffer) noexcept;

	ID3D12Device& mDevice;
	tbb::concurrent_queue<ID3D12CommandList*>& mCmdListQueue;

	ID3D12GraphicsCommandList* mCmdList{ nullptr };
	ID3D12CommandAllocator* mCmdAlloc[Settings::sQueuedFrameCount]{ nullptr };
	std::uint32_t mCurrFrameIndex{ 0U };

	D3D12_CPU_DESCRIPTOR_HANDLE mDepthBufferCpuDesc{ 0UL };

	D3D12_GPU_DESCRIPTOR_HANDLE mColorBufferGpuDescHandle{ 0UL };
};
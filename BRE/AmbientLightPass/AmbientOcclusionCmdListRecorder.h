#pragma once

#include <tbb/concurrent_queue.h>

#include <SettingsManager\SettingsManager.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct FrameCBuffer;
struct ID3D12CommandAllocator;
struct ID3D12CommandList;
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;
class UploadBuffer;

// Responsible of command lists recording to be executed by CommandListExecutor.
// This class has common data and functionality to record command list for ambient occlusion pass.
class AmbientOcclusionCmdListRecorder {
public:
	explicit AmbientOcclusionCmdListRecorder(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);

	~AmbientOcclusionCmdListRecorder() = default;
	AmbientOcclusionCmdListRecorder(const AmbientOcclusionCmdListRecorder&) = delete;
	const AmbientOcclusionCmdListRecorder& operator=(const AmbientOcclusionCmdListRecorder&) = delete;
	AmbientOcclusionCmdListRecorder(AmbientOcclusionCmdListRecorder&&) = default;
	AmbientOcclusionCmdListRecorder& operator=(AmbientOcclusionCmdListRecorder&&) = default;

	// This method is to initialize PSO that is a shared between all this kind
	// of recorders.
	// This method is initialized by its corresponding pass.
	static void InitPSO() noexcept;

	void Init(
		ID3D12Resource& normalSmoothnessBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& ambientAccessBufferCpuDesc,
		ID3D12Resource& depthBuffer) noexcept;

	void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept;

	bool ValidateData() const noexcept;

private:
	void BuildBuffers(
		const void* sampleKernel,
		const void* kernelNoise,
		ID3D12Resource& normalSmoothnessBuffer,
		ID3D12Resource& depthBuffer) noexcept;

	tbb::concurrent_queue<ID3D12CommandList*>& mCmdListQueue;

	ID3D12GraphicsCommandList* mCmdList{ nullptr };
	ID3D12CommandAllocator* mCmdAlloc[SettingsManager::sQueuedFrameCount]{ nullptr };

	std::uint32_t mNumSamples{ 0U };

	UploadBuffer* mFrameCBuffer[SettingsManager::sQueuedFrameCount]{ nullptr };

	UploadBuffer* mSampleKernelBuffer{ nullptr };
	D3D12_GPU_DESCRIPTOR_HANDLE mSampleKernelBufferGpuDescBegin{ 0UL };

	D3D12_CPU_DESCRIPTOR_HANDLE mAmbientAccessBufferCpuDesc{ 0UL };

	// Pixel shader buffers GPU descriptor handle
	D3D12_GPU_DESCRIPTOR_HANDLE mPixelShaderBuffersGpuDesc{ 0UL };
};
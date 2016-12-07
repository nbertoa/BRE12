#pragma once

#include <tbb/concurrent_queue.h>

#include <GlobalData/Settings.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct ID3D12CommandAllocator;
struct ID3D12CommandList;
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;
class UploadBuffer;

// Responsible of command lists recording to be executed by CommandListExecutor.
// This class has common data and functionality to record command list for ambient light pass.
class AmbientLightCmdListRecorder {
public:
	explicit AmbientLightCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);

	~AmbientLightCmdListRecorder() = default;
	AmbientLightCmdListRecorder(const AmbientLightCmdListRecorder&) = delete;
	const AmbientLightCmdListRecorder& operator=(const AmbientLightCmdListRecorder&) = delete;
	AmbientLightCmdListRecorder(AmbientLightCmdListRecorder&&) = default;
	AmbientLightCmdListRecorder& operator=(AmbientLightCmdListRecorder&&) = default;

	// This method is to initialize PSO that is a shared between all this kind
	// of recorders.
	// This method is initialized by its corresponding pass.
	static void InitPSO() noexcept;

	void Init(
		ID3D12Resource& baseColorMetalMaskBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
		ID3D12Resource& ambientAccessibilityBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& ambientAccessibilityBufferRTCpuDesc,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept;

	void RecordAndPushCommandLists() noexcept;

	bool ValidateData() const noexcept;

private:
	void BuildBuffers(
		ID3D12Resource& baseColorMetalMaskBuffer,
		ID3D12Resource& ambientAccessibilityBuffer) noexcept;

	ID3D12Device& mDevice;
	tbb::concurrent_queue<ID3D12CommandList*>& mCmdListQueue;

	ID3D12GraphicsCommandList* mCmdList{ nullptr };
	ID3D12CommandAllocator* mCmdAlloc[Settings::sQueuedFrameCount]{ nullptr };
	std::uint32_t mCurrFrameIndex{ 0U };

	// Buffers cpu descriptors
	D3D12_CPU_DESCRIPTOR_HANDLE mColorBufferCpuDesc{ 0UL };
	D3D12_CPU_DESCRIPTOR_HANDLE mAmbientAccessibilityBufferRTCpuDesc{ 0UL };
	D3D12_CPU_DESCRIPTOR_HANDLE mDepthBufferCpuDesc{ 0UL };

	// BaseColor_MetalMask GPU descriptor handle
	D3D12_GPU_DESCRIPTOR_HANDLE mBaseColor_MetalMaskGpuDescHandle{ 0UL };
};
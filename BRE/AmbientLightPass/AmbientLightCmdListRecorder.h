#pragma once

#include <SettingsManager\SettingsManager.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct ID3D12CommandAllocator;
struct ID3D12CommandList;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;
class UploadBuffer;

// Responsible of command lists recording to be executed by CommandListExecutor.
// This class has common data and functionality to record command list for ambient light pass.
class AmbientLightCmdListRecorder {
public:
	AmbientLightCmdListRecorder();
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
		const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferCpuDesc,
		ID3D12Resource& ambientAccessibilityBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& ambientAccessibilityBufferRTCpuDesc) noexcept;

	// Preconditions:
	// - Init() must be called first
	void RecordAndPushCommandLists() noexcept;

	bool ValidateData() const noexcept;

private:
	void InitShaderResourceViews(
		ID3D12Resource& baseColorMetalMaskBuffer,
		ID3D12Resource& ambientAccessibilityBuffer) noexcept;

	ID3D12GraphicsCommandList* mCommandList{ nullptr };
	ID3D12CommandAllocator* mCommandAllocators[SettingsManager::sQueuedFrameCount]{ nullptr };

	D3D12_CPU_DESCRIPTOR_HANDLE mOutputColorBufferCpuDesc{ 0UL };
	D3D12_CPU_DESCRIPTOR_HANDLE mAmbientAccessibilityBufferRenderTargetCpuDesc{ 0UL };

	D3D12_GPU_DESCRIPTOR_HANDLE mBaseColor_MetalMaskGpuDesc{ 0UL };
};
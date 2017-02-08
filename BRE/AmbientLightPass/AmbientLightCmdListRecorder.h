#pragma once

#include <CommandManager\CommandListPerFrame.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct ID3D12Resource;

// Responsible of command lists recording to be executed by CommandListExecutor.
// This class has common data and functionality to record command list for ambient light pass.
class AmbientLightCmdListRecorder {
public:
	AmbientLightCmdListRecorder() = default;
	~AmbientLightCmdListRecorder() = default;
	AmbientLightCmdListRecorder(const AmbientLightCmdListRecorder&) = delete;
	const AmbientLightCmdListRecorder& operator=(const AmbientLightCmdListRecorder&) = delete;
	AmbientLightCmdListRecorder(AmbientLightCmdListRecorder&&) = default;
	AmbientLightCmdListRecorder& operator=(AmbientLightCmdListRecorder&&) = default;

	static void InitSharedPSOAndRootSignature() noexcept;

	// Preconditions:
	// - InitSharedPSOAndRootSignature() must be called first and once
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
		
	CommandListPerFrame mCommandListPerFrame;

	D3D12_CPU_DESCRIPTOR_HANDLE mOutputColorBufferCpuDescriptor{ 0UL };
	D3D12_CPU_DESCRIPTOR_HANDLE mAmbientAccessibilityBufferRenderTargetCpuDescriptor{ 0UL };

	D3D12_GPU_DESCRIPTOR_HANDLE mBaseColor_MetalMaskGpuDescriptor{ 0UL };
};
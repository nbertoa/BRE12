#pragma once

#include <CommandManager\CommandListPerFrame.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;
struct ID3D12Resource;

class BlurCmdListRecorder {
public:
	BlurCmdListRecorder() = default;
	~BlurCmdListRecorder() = default;
	BlurCmdListRecorder(const BlurCmdListRecorder&) = delete;
	const BlurCmdListRecorder& operator=(const BlurCmdListRecorder&) = delete;
	BlurCmdListRecorder(BlurCmdListRecorder&&) = default;
	BlurCmdListRecorder& operator=(BlurCmdListRecorder&&) = default;

	static void InitSharedPSOAndRootSignature() noexcept;

	// Preconditions:
	// - InitSharedPSOAndRootSignature() must be called first and once
	void Init(
		ID3D12Resource& inputColorBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferCpuDesc) noexcept;

	// Preconditions:
	// - Init() must be called first
	void RecordAndPushCommandLists() noexcept;

	bool ValidateData() const noexcept;

private:
	void InitShaderResourceViews(ID3D12Resource& colorBuffer) noexcept;

	CommandListPerFrame mCommandListPerFrame;

	D3D12_GPU_DESCRIPTOR_HANDLE mInputColorBufferGpuDescriptor{ 0UL };

	D3D12_CPU_DESCRIPTOR_HANDLE mOutputColorBufferCpuDescriptor{ 0UL };
};
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

	// This method is to initialize PSO that is a shared between all this kind
	// of recorders.
	// This method is initialized by its corresponding pass.
	static void InitPSO() noexcept;

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

	D3D12_GPU_DESCRIPTOR_HANDLE mInputColorBufferGpuDesc{ 0UL };

	D3D12_CPU_DESCRIPTOR_HANDLE mOutputColorBufferCpuDesc{ 0UL };
};
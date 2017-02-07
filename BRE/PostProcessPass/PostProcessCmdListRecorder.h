#pragma once

#include <d3d12.h>

#include <SettingsManager\SettingsManager.h>

class UploadBuffer;

// To record command list for post processing effects (anti aliasing, color grading, etc).
class PostProcessCmdListRecorder {
public:
	PostProcessCmdListRecorder();
	~PostProcessCmdListRecorder() = default;
	PostProcessCmdListRecorder(const PostProcessCmdListRecorder&) = delete;
	const PostProcessCmdListRecorder& operator=(const PostProcessCmdListRecorder&) = delete;
	PostProcessCmdListRecorder(PostProcessCmdListRecorder&&) = default;
	PostProcessCmdListRecorder& operator=(PostProcessCmdListRecorder&&) = default;

	// This method is to initialize PSO that is a shared between all this kind
	// of recorders.
	// This method is initialized by its corresponding pass.
	static void InitPSO() noexcept;

	void Init(ID3D12Resource& inputColorBuffer) noexcept;

	// Preconditions:
	// - Init() must be called first
	void RecordAndPushCommandLists(const D3D12_CPU_DESCRIPTOR_HANDLE& frameBufferCpuDesc) noexcept;

	bool IsDataValid() const noexcept;

private:
	void InitShaderResourceViews(ID3D12Resource& inputColorBuffer) noexcept;

	ID3D12GraphicsCommandList* mCommandList{ nullptr };
	ID3D12CommandAllocator* mCommandAllocators[SettingsManager::sQueuedFrameCount]{ nullptr };

	D3D12_GPU_DESCRIPTOR_HANDLE mInputColorBufferGpuDesc{ 0UL };
};
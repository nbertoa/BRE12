#pragma once

#include <memory>

#include <SkyBoxPass\SkyBoxCmdListRecorder.h>

class CommandListProcessor;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct FrameCBuffer;

// Pass that renders the sky box
class SkyBoxPass {
public:
	using Recorder = std::unique_ptr<SkyBoxCmdListRecorder>;

	SkyBoxPass() = default;
	SkyBoxPass(const SkyBoxPass&) = delete;
	const SkyBoxPass& operator=(const SkyBoxPass&) = delete;

	// You should call this method after filling recorder and before Execute()
	void Init(
		ID3D12Device& device,
		ID3D12CommandQueue& cmdQueue,
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		ID3D12Resource& skyBoxCubeMap,
		const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept;

	void Execute(CommandListProcessor& cmdListProcessor, const FrameCBuffer& frameCBuffer) const noexcept;

private:
	// Method used internally for validation purposes
	bool ValidateData() const noexcept;
	
	ID3D12CommandAllocator* mCmdAlloc{ nullptr };
	ID3D12GraphicsCommandList* mCmdList{ nullptr };

	ID3D12Fence* mFence{ nullptr };

	// Color & Depth buffers cpu descriptors
	D3D12_CPU_DESCRIPTOR_HANDLE mColorBufferCpuDesc{ 0UL };
	D3D12_CPU_DESCRIPTOR_HANDLE mDepthBufferCpuDesc{ 0UL };

	Recorder mRecorder;
};

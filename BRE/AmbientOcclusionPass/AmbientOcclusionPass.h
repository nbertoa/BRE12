#pragma once

#include <memory>
#include <tbb/concurrent_queue.h>

#include <AmbientOcclusionPass\AmbientOcclusionCmdListRecorder.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct FrameCBuffer;
struct ID3D12CommandAllocator;
struct ID3D12CommandList;
struct ID3D12CommandQueue;
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;

// Pass responsible to apply ambient occlusion
class AmbientOcclusionPass {
public:
	using Recorder = std::unique_ptr<AmbientOcclusionCmdListRecorder>;

	AmbientOcclusionPass() = default;
	~AmbientOcclusionPass() = default;
	AmbientOcclusionPass(const AmbientOcclusionPass&) = delete;
	const AmbientOcclusionPass& operator=(const AmbientOcclusionPass&) = delete;
	AmbientOcclusionPass(AmbientOcclusionPass&&) = delete;
	AmbientOcclusionPass& operator=(AmbientOcclusionPass&&) = delete;

	// You should call this method before Execute()
	void Init(
		ID3D12Device& device,
		ID3D12CommandQueue& cmdQueue,
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		ID3D12Resource& normalSmoothnessBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& ambientAccessBufferCpuDesc,
		ID3D12Resource& depthBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept;

	void Execute(const FrameCBuffer& frameCBuffer) const noexcept;

private:
	// Method used internally for validation purposes
	bool ValidateData() const noexcept;
	
	ID3D12CommandAllocator* mCmdAlloc{ nullptr };

	ID3D12GraphicsCommandList* mCmdList{ nullptr };

	ID3D12Fence* mFence{ nullptr };

	Recorder mRecorder;
};

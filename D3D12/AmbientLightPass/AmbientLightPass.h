#pragma once

#include <memory>
#include <tbb/concurrent_queue.h>

#include <AmbientLightPass\AmbientCmdListRecorder.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct ID3D12CommandAllocator;
struct ID3D12CommandList;
struct ID3D12CommandQueue;
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;

// Pass responsible to apply ambient lighting and ambient occlusion
class AmbientLightPass {
public:
	using Recorder = std::unique_ptr<AmbientCmdListRecorder>;

	AmbientLightPass() = default;
	AmbientLightPass(const AmbientLightPass&) = delete;
	const AmbientLightPass& operator=(const AmbientLightPass&) = delete;

	// You should call this method before Execute()
	void Init(
		ID3D12Device& device,
		ID3D12CommandQueue& cmdQueue,
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		ID3D12Resource& baseColorMetalMaskBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept;

	void Execute() const noexcept;

private:
	// Method used internally for validation purposes
	bool ValidateData() const noexcept;
	
	ID3D12CommandAllocator* mCmdAlloc{ nullptr };

	ID3D12GraphicsCommandList* mCmdList{ nullptr };

	ID3D12Fence* mFence{ nullptr };

	Recorder mRecorder;
};

#pragma once

#include <memory>
#include <tbb/concurrent_queue.h>

#include <EnvironmentLightPass\EnvironmentLightCmdListRecorder.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct FrameCBuffer;
struct ID3D12CommandAllocator;
struct ID3D12CommandList;
struct ID3D12CommandQueue;
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;

// Pass responsible to apply diffuse irradiance & specular pre-convolved environment cube maps
class EnvironmentLightPass {
public:
	using Recorder = std::unique_ptr<EnvironmentLightCmdListRecorder>;

	EnvironmentLightPass() = default;
	EnvironmentLightPass(const EnvironmentLightPass&) = delete;
	const EnvironmentLightPass& operator=(const EnvironmentLightPass&) = delete;

	// You should call this method before Execute()
	void Init(
		ID3D12Device& device,
		ID3D12CommandQueue& cmdQueue,
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc,
		ID3D12Resource& diffuseIrradianceCubeMap,
		ID3D12Resource& specularPreConvolvedCubeMap) noexcept;

	void Execute(const FrameCBuffer& frameCBuffer) const noexcept;

private:
	// Method used internally for validation purposes
	bool ValidateData() const noexcept;
	
	ID3D12CommandAllocator* mCmdAlloc{ nullptr };

	ID3D12GraphicsCommandList* mCmdList{ nullptr };

	ID3D12Fence* mFence{ nullptr };

	Recorder mRecorder;
};

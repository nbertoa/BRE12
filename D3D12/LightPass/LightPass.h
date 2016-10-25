#pragma once

#include <memory>
#include <tbb/concurrent_queue.h>
#include <vector>
#include <wrl.h>

#include <AmbientPass\AmbientPass.h>
#include <EnvironmentLightPass/EnvironmentLightPass.h>
#include <GlobalData\Settings.h>
#include <LightPass\LightPassCmdListRecorder.h>

class CommandListProcessor;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct FrameCBuffer;
struct ID3D12CommandAllocator;
struct ID3D12CommandQueue;
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;

// Pass responsible to execute recorders related with deferred shading lighting pass
class LightPass {
public:
	using Recorders = std::vector<std::unique_ptr<LightPassCmdListRecorder>>;

	LightPass() = default;
	LightPass(const LightPass&) = delete;
	const LightPass& operator=(const LightPass&) = delete;

	// You should get recorders and fill them, before calling Init()
	__forceinline Recorders& GetRecorders() noexcept { return mRecorders; }

	// You should call this method after filling recorders and before Execute()
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

	void Execute(
		CommandListProcessor& cmdListProcessor,
		ID3D12CommandQueue& cmdQueue,
		const FrameCBuffer& frameCBuffer) noexcept;

private:
	// Method used internally for validation purposes
	bool ValidateData() const noexcept;

	// 1 command allocater per queued frame.	
	ID3D12CommandAllocator* mCmdAllocsBegin[Settings::sQueuedFrameCount]{ nullptr };
	ID3D12CommandAllocator* mCmdAllocsEnd[Settings::sQueuedFrameCount]{ nullptr };

	ID3D12GraphicsCommandList* mCmdList{ nullptr };

	// Geometry buffers created by GeometryPass
	Microsoft::WRL::ComPtr<ID3D12Resource>* mGeometryBuffers;

	ID3D12Resource* mDepthBuffer{ nullptr };

	// Color & Depth buffers cpu descriptors
	D3D12_CPU_DESCRIPTOR_HANDLE mColorBufferCpuDesc{ 0UL };
	D3D12_CPU_DESCRIPTOR_HANDLE mDepthBufferCpuDesc{ 0UL };

	Recorders mRecorders;

	AmbientPass mAmbientPass;
	EnvironmentLightPass mEnvironmentLightPass;
};

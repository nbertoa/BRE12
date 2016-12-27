#pragma once

#include <memory>
#include <tbb/concurrent_queue.h>
#include <vector>
#include <wrl.h>

#include <AmbientLightPass\AmbientLightPass.h>
#include <EnvironmentLightPass/EnvironmentLightPass.h>
#include <LightingPass\LightingPassCmdListRecorder.h>
#include <SettingsManager\SettingsManager.h>

class CommandListExecutor;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct FrameCBuffer;
struct ID3D12CommandAllocator;
struct ID3D12CommandQueue;
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;

// Pass responsible to execute recorders related with deferred shading lighting pass
class LightingPass {
public:
	using Recorders = std::vector<std::unique_ptr<LightingPassCmdListRecorder>>;

	LightingPass() = default;
	~LightingPass() = default;
	LightingPass(const LightingPass&) = delete;
	const LightingPass& operator=(const LightingPass&) = delete;
	LightingPass(LightingPass&&) = delete;
	LightingPass& operator=(LightingPass&&) = delete;

	// You should get recorders and fill them, before calling Init()
	__forceinline Recorders& GetRecorders() noexcept { return mRecorders; }

	// You should call this method after filling recorders and before Execute()
	void Init(
		CommandListExecutor& cmdListExecutor,
		ID3D12CommandQueue& cmdQueue,
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers, 
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
		ID3D12Resource& diffuseIrradianceCubeMap,
		ID3D12Resource& specularPreConvolvedCubeMap) noexcept;

	void Execute(const FrameCBuffer& frameCBuffer) noexcept;

private:
	// Method used internally for validation purposes
	bool ValidateData() const noexcept;

	void ExecuteBeginTask() noexcept;
	void ExecuteEndingTask() noexcept;

	CommandListExecutor* mCmdListExecutor{ nullptr };
	ID3D12CommandQueue* mCmdQueue{ nullptr };

	// 1 command allocater per queued frame.	
	ID3D12CommandAllocator* mCmdAllocsBegin[SettingsManager::sQueuedFrameCount]{ nullptr };
	ID3D12CommandAllocator* mCmdAllocsEnd[SettingsManager::sQueuedFrameCount]{ nullptr };

	ID3D12GraphicsCommandList* mCmdList{ nullptr };

	// Geometry buffers created by GeometryPass
	Microsoft::WRL::ComPtr<ID3D12Resource>* mGeometryBuffers;

	ID3D12Resource* mDepthBuffer{ nullptr };

	D3D12_CPU_DESCRIPTOR_HANDLE mColorBufferCpuDesc{ 0UL };

	Recorders mRecorders;

	AmbientLightPass mAmbientLightPass;
	EnvironmentLightPass mEnvironmentLightPass;
};

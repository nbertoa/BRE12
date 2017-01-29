#pragma once

#include <memory>
#include <vector>
#include <wrl.h>

#include <AmbientLightPass\AmbientLightPass.h>
#include <EnvironmentLightPass/EnvironmentLightPass.h>
#include <LightingPass\LightingPassCmdListRecorder.h>
#include <SettingsManager\SettingsManager.h>

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
	using CommandListRecorders = std::vector<std::unique_ptr<LightingPassCmdListRecorder>>;

	LightingPass() = default;
	~LightingPass() = default;
	LightingPass(const LightingPass&) = delete;
	const LightingPass& operator=(const LightingPass&) = delete;
	LightingPass(LightingPass&&) = delete;
	LightingPass& operator=(LightingPass&&) = delete;

	// You should get recorders and fill them, before calling Init()
	__forceinline CommandListRecorders& GetCommandListRecorders() noexcept { return mRecorders; }

	// Preconditions:
	// - "geometryBuffers" must not be nullptr
	// - "geometryBuffersCount" must be greater than zero
	void Init(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers, 
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
		ID3D12Resource& diffuseIrradianceCubeMap,
		ID3D12Resource& specularPreConvolvedCubeMap) noexcept;

	// Preconditions:
	// - Init() must be called first
	void Execute(const FrameCBuffer& frameCBuffer) noexcept;

private:
	// Method used internally for validation purposes
	bool IsDataValid() const noexcept;

	void ExecuteBeginTask() noexcept;
	void ExecuteFinalTask() noexcept;

	// 1 command allocater per queued frame.	
	ID3D12CommandAllocator* mCmdAllocatorsBegin[SettingsManager::sQueuedFrameCount]{ nullptr };
	ID3D12CommandAllocator* mCmdAllocatorsFinal[SettingsManager::sQueuedFrameCount]{ nullptr };

	ID3D12GraphicsCommandList* mCommandList{ nullptr };

	// Geometry buffers created by GeometryPass
	Microsoft::WRL::ComPtr<ID3D12Resource>* mGeometryBuffers;

	ID3D12Resource* mDepthBuffer{ nullptr };

	D3D12_CPU_DESCRIPTOR_HANDLE mOutputColorBufferCpuDesc{ 0UL };

	CommandListRecorders mRecorders;

	AmbientLightPass mAmbientLightPass;
	EnvironmentLightPass mEnvironmentLightPass;
};

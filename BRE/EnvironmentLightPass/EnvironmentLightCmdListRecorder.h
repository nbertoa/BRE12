#pragma once

#include <wrl.h>

#include <SettingsManager\SettingsManager.h>

class UploadBuffer;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct FrameCBuffer;
struct ID3D12CommandAllocator;
struct ID3D12CommandList;
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;

// Responsible of command lists recording to be executed by CommandListExecutor.
// This class has common data and functionality to record command list for environment light pass.
class EnvironmentLightCmdListRecorder {
public:
	EnvironmentLightCmdListRecorder();
	~EnvironmentLightCmdListRecorder() = default;
	EnvironmentLightCmdListRecorder(const EnvironmentLightCmdListRecorder&) = delete;
	const EnvironmentLightCmdListRecorder& operator=(const EnvironmentLightCmdListRecorder&) = delete;
	EnvironmentLightCmdListRecorder(EnvironmentLightCmdListRecorder&&) = default;
	EnvironmentLightCmdListRecorder& operator=(EnvironmentLightCmdListRecorder&&) = default;

	// This method is to initialize PSO that is a shared between all this kind
	// of recorders.
	// This method is initialized by its corresponding pass.
	static void InitPSO() noexcept;

	void Init(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferCpuDesc,
		ID3D12Resource& diffuseIrradianceCubeMap,
		ID3D12Resource& specularPreConvolvedCubeMap) noexcept;

	// Preconditions:
	// - Init() must be called first
	void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept;

	bool ValidateData() const noexcept;

private:
	void BuildBuffers(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers, 
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer,
		ID3D12Resource& diffuseIrradianceCubeMap,
		ID3D12Resource& specularPreConvolvedCubeMap) noexcept;

	ID3D12GraphicsCommandList* mCommandList{ nullptr };
	ID3D12CommandAllocator* mCommandAllocators[SettingsManager::sQueuedFrameCount]{ nullptr };

	UploadBuffer* mFrameCBuffer[SettingsManager::sQueuedFrameCount]{ nullptr };

	D3D12_GPU_DESCRIPTOR_HANDLE mCubeMapsBufferGpuDescBegin;

	D3D12_CPU_DESCRIPTOR_HANDLE mOutputColorBufferCpuDesc{ 0UL };

	D3D12_GPU_DESCRIPTOR_HANDLE mPixelShaderBuffersGpuDesc{ 0UL };
};
#pragma once

#include <tbb/concurrent_queue.h>
#include <wrl.h>

#include <GlobalData/Settings.h>

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
	explicit EnvironmentLightCmdListRecorder(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);

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
		const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
		ID3D12Resource& diffuseIrradianceCubeMap,
		ID3D12Resource& specularPreConvolvedCubeMap) noexcept;

	void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept;

	bool ValidateData() const noexcept;

private:
	void BuildBuffers(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers, 
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer,
		ID3D12Resource& diffuseIrradianceCubeMap,
		ID3D12Resource& specularPreConvolvedCubeMap) noexcept;

	tbb::concurrent_queue<ID3D12CommandList*>& mCmdListQueue;

	ID3D12GraphicsCommandList* mCmdList{ nullptr };
	ID3D12CommandAllocator* mCmdAlloc[Settings::sQueuedFrameCount]{ nullptr };

	UploadBuffer* mFrameCBuffer[Settings::sQueuedFrameCount]{ nullptr };

	D3D12_GPU_DESCRIPTOR_HANDLE mCubeMapsBufferGpuDescBegin;

	D3D12_CPU_DESCRIPTOR_HANDLE mColorBufferCpuDesc{ 0UL };

	// Textures GPU descriptor handle
	D3D12_GPU_DESCRIPTOR_HANDLE mTexturesGpuDesc{ 0UL };
};
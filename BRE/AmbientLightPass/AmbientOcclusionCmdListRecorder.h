#pragma once

#include <CommandManager\CommandListPerFrame.h>
#include <ResourceManager\FrameCBufferPerFrame.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct FrameCBuffer;
struct ID3D12Resource;

// Responsible of command lists recording to be executed by CommandListExecutor.
// This class has common data and functionality to record command list for ambient occlusion pass.
class AmbientOcclusionCmdListRecorder {
public:
	AmbientOcclusionCmdListRecorder() = default;
	~AmbientOcclusionCmdListRecorder() = default;
	AmbientOcclusionCmdListRecorder(const AmbientOcclusionCmdListRecorder&) = delete;
	const AmbientOcclusionCmdListRecorder& operator=(const AmbientOcclusionCmdListRecorder&) = delete;
	AmbientOcclusionCmdListRecorder(AmbientOcclusionCmdListRecorder&&) = default;
	AmbientOcclusionCmdListRecorder& operator=(AmbientOcclusionCmdListRecorder&&) = default;

	// This method is to initialize PSO that is a shared between all this kind
	// of recorders.
	// This method is initialized by its corresponding pass.
	static void InitPSO() noexcept;

	void Init(
		ID3D12Resource& normalSmoothnessBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& ambientAccessibilityBufferCpuDesc,
		ID3D12Resource& depthBuffer) noexcept;

	// Preconditions:
	// - Init() must be called first
	void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept;

	bool ValidateData() const noexcept;

private:
	void CreateSampleKernelBuffer(const void* randomSamples) noexcept;
	ID3D12Resource* CreateAndGetNoiseTexture(const void* noiseVectors) noexcept;
	void InitShaderResourceViews(
		ID3D12Resource& normalSmoothnessBuffer,
		ID3D12Resource& depthBuffer,
		ID3D12Resource& noiseTexture) noexcept;
	
	CommandListPerFrame mCommandListPerFrame;

	std::uint32_t mSampleKernelSize{ 0U };
	std::uint32_t mNoiseTextureDimension{ 4U };

	FrameCBufferPerFrame mFrameCBufferPerFrame;

	UploadBuffer* mSampleKernelBuffer{ nullptr };
	D3D12_GPU_DESCRIPTOR_HANDLE mSampleKernelBufferGpuDescBegin{ 0UL };

	D3D12_CPU_DESCRIPTOR_HANDLE mAmbientAccessibilityBufferCpuDesc{ 0UL };

	D3D12_GPU_DESCRIPTOR_HANDLE mPixelShaderBuffersGpuDesc{ 0UL };
};
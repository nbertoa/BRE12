#pragma once

#include <wrl.h>

#include <CommandManager\CommandListPerFrame.h>
#include <ResourceManager\FrameUploadCBufferPerFrame.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct FrameCBuffer;
struct ID3D12Resource;

// Responsible of command lists recording to be executed by CommandListExecutor.
// This class has common data and functionality to record command list for environment light pass.
class EnvironmentLightCmdListRecorder {
public:
	EnvironmentLightCmdListRecorder() = default;
	~EnvironmentLightCmdListRecorder() = default;
	EnvironmentLightCmdListRecorder(const EnvironmentLightCmdListRecorder&) = delete;
	const EnvironmentLightCmdListRecorder& operator=(const EnvironmentLightCmdListRecorder&) = delete;
	EnvironmentLightCmdListRecorder(EnvironmentLightCmdListRecorder&&) = default;
	EnvironmentLightCmdListRecorder& operator=(EnvironmentLightCmdListRecorder&&) = default;

	static void InitSharedPSOAndRootSignature() noexcept;

	// Preconditions:
	// - InitSharedPSOAndRootSignature() must be called first and once
	void Init(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer,		
		ID3D12Resource& diffuseIrradianceCubeMap,
		ID3D12Resource& specularPreConvolvedCubeMap,
		const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

	// Preconditions:
	// - Init() must be called first
	void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept;

	bool ValidateData() const noexcept;

private:
	void InitShaderResourceViews(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer,
		ID3D12Resource& diffuseIrradianceCubeMap,
		ID3D12Resource& specularPreConvolvedCubeMap) noexcept;
		
	CommandListPerFrame mCommandListPerFrame;

	FrameUploadCBufferPerFrame mFrameUploadCBufferPerFrame;

	D3D12_CPU_DESCRIPTOR_HANDLE mRenderTargetView{ 0UL };

	D3D12_GPU_DESCRIPTOR_HANDLE mStartPixelShaderResourceView{ 0UL };
};
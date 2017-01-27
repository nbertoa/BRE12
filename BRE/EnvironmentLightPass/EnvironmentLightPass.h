#pragma once

#include <memory>

#include <EnvironmentLightPass\EnvironmentLightCmdListRecorder.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct FrameCBuffer;
struct ID3D12CommandAllocator;
struct ID3D12CommandList;
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;

// Pass responsible to apply diffuse irradiance & specular pre-convolved environment cube maps
class EnvironmentLightPass {
public:
	using Recorder = std::unique_ptr<EnvironmentLightCmdListRecorder>;

	EnvironmentLightPass() = default;
	~EnvironmentLightPass() = default;
	EnvironmentLightPass(const EnvironmentLightPass&) = delete;
	const EnvironmentLightPass& operator=(const EnvironmentLightPass&) = delete;
	EnvironmentLightPass(EnvironmentLightPass&&) = delete;
	EnvironmentLightPass& operator=(EnvironmentLightPass&&) = delete;

	void Init(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferCpuDesc,
		ID3D12Resource& diffuseIrradianceCubeMap,
		ID3D12Resource& specularPreConvolvedCubeMap) noexcept;

	// Preconditions:
	// - Init() must be called first
	void Execute(const FrameCBuffer& frameCBuffer) const noexcept;

private:
	// Method used internally for validation purposes
	bool ValidateData() const noexcept;
	
	ID3D12CommandAllocator* mCommandAllocators{ nullptr };

	ID3D12GraphicsCommandList* mCommandList{ nullptr };

	ID3D12Fence* mFence{ nullptr };

	Recorder mRecorder;
};

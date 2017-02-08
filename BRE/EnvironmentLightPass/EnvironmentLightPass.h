#pragma once

#include <memory>

#include <EnvironmentLightPass\EnvironmentLightCmdListRecorder.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct FrameCBuffer;
struct ID3D12Resource;

// Pass responsible to apply diffuse irradiance & specular pre-convolved environment cube maps
class EnvironmentLightPass {
public:
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
		ID3D12Resource& diffuseIrradianceCubeMap,
		ID3D12Resource& specularPreConvolvedCubeMap,
		const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferCpuDesc) noexcept;

	// Preconditions:
	// - Init() must be called first
	void Execute(const FrameCBuffer& frameCBuffer) const noexcept;

private:
	// Method used internally for validation purposes
	bool ValidateData() const noexcept;

	std::unique_ptr<EnvironmentLightCmdListRecorder> mCommandListRecorder;
};

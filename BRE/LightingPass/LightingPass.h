#pragma once

#include <memory>
#include <vector>
#include <wrl.h>

#include <EnvironmentLightPass\EnvironmentLightPass.h>
#include <CommandManager\CommandListPerFrame.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct FrameCBuffer;
struct ID3D12Resource;

// Pass responsible to execute recorders related with deferred shading lighting pass
class LightingPass {
public:
	LightingPass() = default;
	~LightingPass() = default;
	LightingPass(const LightingPass&) = delete;
	const LightingPass& operator=(const LightingPass&) = delete;
	LightingPass(LightingPass&&) = delete;
	LightingPass& operator=(LightingPass&&) = delete;
	
	// Preconditions:
	// - "geometryBuffers" must not be nullptr
	// - "geometryBuffersCount" must be greater than zero
	void Init(
		ID3D12Resource& baseColorMetalMaskBuffer,
		ID3D12Resource& normalSmoothnessBuffer,
		ID3D12Resource& depthBuffer,		
		ID3D12Resource& diffuseIrradianceCubeMap,
		ID3D12Resource& specularPreConvolvedCubeMap,
		const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

	// Preconditions:
	// - Init() must be called first
	void Execute(const FrameCBuffer& frameCBuffer) noexcept;

private:
	// Method used internally for validation purposes
	bool IsDataValid() const noexcept;

	void ExecuteBeginTask() noexcept;
	void ExecuteFinalTask() noexcept;

	CommandListPerFrame mBeginCommandListPerFrame;
	CommandListPerFrame mFinalCommandListPerFrame;

	// Geometry buffers created by GeometryPass
	Microsoft::WRL::ComPtr<ID3D12Resource>* mGeometryBuffers;

	ID3D12Resource* mBaseColorMetalMaskBuffer{ nullptr };
	ID3D12Resource* mNormalSmoothnessBuffer{ nullptr };
	ID3D12Resource* mDepthBuffer{ nullptr };

	D3D12_CPU_DESCRIPTOR_HANDLE mRenderTargetView{ 0UL };

	EnvironmentLightPass mEnvironmentLightPass;
};

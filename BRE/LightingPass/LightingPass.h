#pragma once

#include <memory>
#include <vector>
#include <wrl.h>

#include <AmbientLightPass\AmbientLightPass.h>
#include <CommandManager\CommandListPerFrame.h>
#include <EnvironmentLightPass/EnvironmentLightPass.h>
#include <LightingPass\LightingPassCmdListRecorder.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct FrameCBuffer;
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
	__forceinline CommandListRecorders& GetCommandListRecorders() noexcept { return mCommandListRecorders; }

	// Preconditions:
	// - "geometryBuffers" must not be nullptr
	// - "geometryBuffersCount" must be greater than zero
	void Init(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers, 
		const std::uint32_t geometryBuffersCount,
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

	ID3D12Resource* mDepthBuffer{ nullptr };

	D3D12_CPU_DESCRIPTOR_HANDLE mRenderTargetView{ 0UL };

	AmbientLightPass mAmbientLightPass;
	EnvironmentLightPass mEnvironmentLightPass;

	CommandListRecorders mCommandListRecorders;
};

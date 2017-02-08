#pragma once

#include <memory>
#include <wrl.h>

#include <AmbientLightPass\AmbientLightCmdListRecorder.h>
#include <AmbientLightPass\AmbientOcclusionCmdListRecorder.h>
#include <AmbientLightPass\BlurCmdListRecorder.h>
#include <CommandManager\CommandListPerFrame.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct ID3D12Resource;

// Pass responsible to apply ambient lighting and ambient occlusion
class AmbientLightPass {
public:
	AmbientLightPass() = default;
	~AmbientLightPass() = default;
	AmbientLightPass(const AmbientLightPass&) = delete;
	const AmbientLightPass& operator=(const AmbientLightPass&) = delete;
	AmbientLightPass(AmbientLightPass&&) = delete;
	AmbientLightPass& operator=(AmbientLightPass&&) = delete;

	void Init(
		ID3D12Resource& baseColorMetalMaskBuffer,
		ID3D12Resource& normalSmoothnessBuffer,		
		ID3D12Resource& depthBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

	// Preconditions:
	// - Init() must be called first
	void Execute(const FrameCBuffer& frameCBuffer) noexcept;

private:
	// Method used internally for validation purposes
	bool ValidateData() const noexcept;

	void ExecuteBeginTask() noexcept;
	void ExecuteMiddleTask() noexcept;
	void ExecuteFinalTask() noexcept;
	
	CommandListPerFrame mBeginCommandListPerFrame;

	CommandListPerFrame mMiddleCommandListPerFrame;

	CommandListPerFrame mFinalCommandListPerFrame;

	Microsoft::WRL::ComPtr<ID3D12Resource> mAmbientAccessibilityBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE mAmbientAccessibilityBufferRenderTargetView{ 0UL };

	Microsoft::WRL::ComPtr<ID3D12Resource> mBlurBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE mBlurBufferRenderTargetView{ 0UL };

	std::unique_ptr<AmbientOcclusionCmdListRecorder> mAmbientOcclusionRecorder;
	std::unique_ptr<AmbientLightCmdListRecorder> mAmbientLightRecorder;
	std::unique_ptr<BlurCmdListRecorder> mBlurRecorder;
};

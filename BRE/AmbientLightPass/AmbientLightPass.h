#pragma once

#include <memory>
#include <wrl.h>

#include <AmbientLightPass\AmbientLightCmdListRecorder.h>
#include <AmbientLightPass\AmbientOcclusionCmdListRecorder.h>
#include <AmbientLightPass\BlurCmdListRecorder.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct ID3D12CommandAllocator;
struct ID3D12CommandList;
struct ID3D12CommandQueue;
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
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
		const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferCpuDesc,
		ID3D12Resource& depthBuffer) noexcept;

	// Preconditions:
	// - Init() must be called first
	void Execute(const FrameCBuffer& frameCBuffer) noexcept;

private:
	// Method used internally for validation purposes
	bool ValidateData() const noexcept;

	void ExecuteBeginTask() noexcept;
	void ExecuteMiddleTask() noexcept;
	void ExecuteFinalTask() noexcept;
	
	// 1 command allocater per queued frame.	
	ID3D12CommandAllocator* mCommandAllocatorsBegin[SettingsManager::sQueuedFrameCount]{ nullptr };
	ID3D12GraphicsCommandList* mCommandListBegin{ nullptr };

	// 1 command allocater per queued frame.
	ID3D12CommandAllocator* mCommandAllocatorsMiddle[SettingsManager::sQueuedFrameCount]{ nullptr };
	ID3D12GraphicsCommandList* mCommandListMiddle{ nullptr };

	// 1 command allocater per queued frame.
	ID3D12CommandAllocator* mCommandAllocatorsFinal[SettingsManager::sQueuedFrameCount]{ nullptr };	
	ID3D12GraphicsCommandList* mCommandListEnd{ nullptr };

	Microsoft::WRL::ComPtr<ID3D12Resource> mAmbientAccessibilityBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE mAmbientAccessibilityBufferRenderTargetCpuDescriptor{ 0UL };

	Microsoft::WRL::ComPtr<ID3D12Resource> mBlurBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE mBlurBufferRenderTargetCpuDescriptor{ 0UL };

	std::unique_ptr<AmbientOcclusionCmdListRecorder> mAmbientOcclusionRecorder;
	std::unique_ptr<AmbientLightCmdListRecorder> mAmbientLightRecorder;
	std::unique_ptr<BlurCmdListRecorder> mBlurRecorder;
};

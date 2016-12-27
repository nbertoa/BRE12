#pragma once

#include <memory>
#include <tbb/concurrent_queue.h>
#include <wrl.h>

#include <AmbientLightPass\AmbientLightCmdListRecorder.h>
#include <AmbientLightPass\AmbientOcclusionCmdListRecorder.h>
#include <AmbientLightPass\BlurCmdListRecorder.h>

class CommandListExecutor;
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

	// You should call this method before Execute()
	void Init(
		CommandListExecutor& cmdListExecutor,
		ID3D12CommandQueue& cmdQueue,
		ID3D12Resource& baseColorMetalMaskBuffer,
		ID3D12Resource& normalSmoothnessBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
		ID3D12Resource& depthBuffer) noexcept;

	void Execute(const FrameCBuffer& frameCBuffer) noexcept;

private:
	// Method used internally for validation purposes
	bool ValidateData() const noexcept;

	void ExecuteBeginTask() noexcept;
	void ExecuteEndingTask() noexcept;

	ID3D12CommandQueue* mCmdQueue{ nullptr };
	
	// 1 command allocater per queued frame.	
	ID3D12CommandAllocator* mCmdAllocsBegin[Settings::sQueuedFrameCount]{ nullptr };
	ID3D12CommandAllocator* mCmdAllocsEnd[Settings::sQueuedFrameCount]{ nullptr };

	ID3D12GraphicsCommandList* mCmdListBegin{ nullptr };
	ID3D12GraphicsCommandList* mCmdListEnd{ nullptr };

	std::unique_ptr<AmbientOcclusionCmdListRecorder> mAmbientOcclusionRecorder;
	std::unique_ptr<AmbientLightCmdListRecorder> mAmbientLightRecorder;	
	std::unique_ptr<BlurCmdListRecorder> mBlurRecorder;

	Microsoft::WRL::ComPtr<ID3D12Resource> mAmbientAccessibilityBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE mAmbientAccessibilityBufferRTCpuDesc{ 0UL };

	Microsoft::WRL::ComPtr<ID3D12Resource> mBlurBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE mBlurBufferRTCpuDesc{ 0UL };

	CommandListExecutor* mCmdListExecutor{ nullptr };
};

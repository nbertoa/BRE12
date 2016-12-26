#pragma once

#include <LightingPass/LightingPassCmdListRecorder.h>

class PunctualLightCmdListRecorder : public LightingPassCmdListRecorder {
public:
	PunctualLightCmdListRecorder() = default;
	~PunctualLightCmdListRecorder() = default;
	PunctualLightCmdListRecorder(const PunctualLightCmdListRecorder&) = delete;
	const PunctualLightCmdListRecorder& operator=(const PunctualLightCmdListRecorder&) = delete;
	PunctualLightCmdListRecorder(PunctualLightCmdListRecorder&&) = default;
	PunctualLightCmdListRecorder& operator=(PunctualLightCmdListRecorder&&) = default;
	
	// This method is to initialize PSO that is a shared between all this kind
	// of recorders.
	// This method is initialized by its corresponding pass.
	static void InitPSO() noexcept;

	// This method must be called before RecordAndPushCommandLists()
	void Init(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer,
		const void* lights,
		const std::uint32_t numLights) noexcept final override;

	// Record command lists and push them to the queue.
	void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept final override;

	bool ValidateData() const noexcept override;

private:
	void BuildBuffers(const void* lights) noexcept;

	D3D12_GPU_DESCRIPTOR_HANDLE mTexturesGpuDescHandle{ 0UL };
};
#pragma once

#include <LightingPass/LightingPassCmdListRecorder.h>

class PunctualLightCmdListRecorder : public LightingPassCmdListRecorder {
public:
	explicit PunctualLightCmdListRecorder(ID3D12Device& device);

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
		const std::uint32_t numLights) noexcept override;

	// Record command lists and push them to the queue.
	void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept override;

private:
	void BuildLightsBuffers(const void* lights, const std::uint32_t numDescriptors) noexcept;
};
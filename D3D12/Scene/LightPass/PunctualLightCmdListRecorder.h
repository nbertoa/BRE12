#pragma once

#include <DirectXMath.h>

#include <Scene/LightPassCmdListRecorder.h>

struct PunctualLight;

class PunctualLightCmdListRecorder : public LightPassCmdListRecorder {
public:
	explicit PunctualLightCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);

	void Init(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		const PunctualLight* lights,
		const std::uint32_t numLights) noexcept;

	void RecordCommandLists(
		const FrameCBuffer& frameCBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE* rtvCpuDescHandles,
		const std::uint32_t rtvCpuDescHandlesCount,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept override;

private:
	void BuildBuffers(const PunctualLight* lights, const std::uint32_t descHeapOffset) noexcept;
};
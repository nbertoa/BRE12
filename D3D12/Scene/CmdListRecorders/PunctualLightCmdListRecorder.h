#pragma once

#include <DirectXMath.h>

#include <Scene/CmdListRecorder.h>

struct PunctualLight;

class PunctualLightCmdListRecorder : public CmdListRecorder {
public:
	explicit PunctualLightCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);

	void Init(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		const PunctualLight* lights,
		const std::uint32_t numLights) noexcept;

	void RecordCommandLists(
		const DirectX::XMFLOAT4X4& view,
		const DirectX::XMFLOAT4X4& proj,
		const DirectX::XMFLOAT3& eyePosW,
		const D3D12_CPU_DESCRIPTOR_HANDLE* rtvCpuDescHandles,
		const std::uint32_t rtvCpuDescHandlesCount,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept override;

	bool ValidateData() const noexcept override;

private:
	void BuildBuffers(const PunctualLight* lights, const std::uint32_t descHeapOffset) noexcept;

	std::uint32_t mNumLights{ 0U };

	D3D12_GPU_DESCRIPTOR_HANDLE mGeometryBuffersGpuDescHandleBegin;

	UploadBuffer* mFrameCBuffer[Settings::sQueuedFrameCount]{ nullptr };
	UploadBuffer* mImmutableCBuffer{ nullptr };

	UploadBuffer* mLightsBuffer{ nullptr };
	D3D12_GPU_DESCRIPTOR_HANDLE mLightsBufferGpuDescHandleBegin;
};
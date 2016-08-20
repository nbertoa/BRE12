#pragma once

#include <DirectXMath.h>

#include <Scene/CmdListRecorder.h>

class PunctualLightCmdListRecorder : public CmdListRecorder {
public:
	explicit PunctualLightCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);

	__forceinline static std::uint32_t MaxNumLights() noexcept { return sMaxNumLights; }
	__forceinline std::uint32_t& NumLights() noexcept { return mNumLights; }

	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE& GeometryBuffersGpuDescHandleBegin() noexcept { return mGeometryBuffersGpuDescHandleBegin; }

	__forceinline UploadBuffer* &FrameCBuffer() noexcept { return mFrameCBuffer; }

	__forceinline UploadBuffer* &LightsBuffer() noexcept { return mLightsBuffer; }
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE& LightsBufferGpuDescHandleBegin() noexcept { return mLightsBufferGpuDescHandleBegin; }

	void RecordCommandLists(
		const DirectX::XMFLOAT4X4& view,
		const DirectX::XMFLOAT4X4& proj,
		const D3D12_CPU_DESCRIPTOR_HANDLE* geomPassRtvCpuDescHandles,
		const std::uint32_t geomPassRtvCpuDescHandlesCount,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept override;

	bool ValidateData() const noexcept override;

private:
	static const std::uint32_t sMaxNumLights{ 250U };

	std::uint32_t mNumLights{ 0U };

	D3D12_GPU_DESCRIPTOR_HANDLE mGeometryBuffersGpuDescHandleBegin;

	UploadBuffer* mFrameCBuffer{ nullptr };

	UploadBuffer* mLightsBuffer{ nullptr };
	D3D12_GPU_DESCRIPTOR_HANDLE mLightsBufferGpuDescHandleBegin;
};
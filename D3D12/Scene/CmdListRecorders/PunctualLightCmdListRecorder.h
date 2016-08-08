#pragma once

#include <DirectXMath.h>

#include <Scene/CmdListRecorder.h>

class PunctualLightCmdListRecorder : public CmdListRecorder {
public:
	explicit PunctualLightCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);

	__forceinline Matrices& WorldMatrices() noexcept { return mWorldMatrices; }
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE& GeometryBuffersGpuDescHandleBegin() noexcept { return mGeometryBuffersGpuDescHandleBegin; }

	void RecordCommandLists(
		const DirectX::XMFLOAT4X4& view,
		const DirectX::XMFLOAT4X4& proj,
		const D3D12_CPU_DESCRIPTOR_HANDLE* geomPassRtvCpuDescHandles,
		const std::uint32_t geomPassRtvCpuDescHandlesCount,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept override;

	bool ValidateData() const noexcept override;

private:
	Matrices mWorldMatrices;

	D3D12_GPU_DESCRIPTOR_HANDLE mGeometryBuffersGpuDescHandleBegin;
};
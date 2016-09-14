#pragma once

#include <DirectXMath.h>

#include <Scene/CmdListRecorder.h>

class SkyBoxCmdListRecorder : public CmdListRecorder {
public:
	explicit SkyBoxCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);

	void Init(const GeometryData& geometryData, ID3D12Resource& cubeMap) noexcept;

	void RecordCommandLists(
		const DirectX::XMFLOAT4X4& view,
		const DirectX::XMFLOAT4X4& proj,
		const DirectX::XMFLOAT3& eyePosW,
		const D3D12_CPU_DESCRIPTOR_HANDLE* rtvCpuDescHandles,
		const std::uint32_t rtvCpuDescHandlesCount,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept override;

	bool ValidateData() const noexcept;

private:
	void BuildBuffers(ID3D12Resource& cubeMap) noexcept;

	GeometryData mGeometryData;

	UploadBuffer* mFrameCBuffer[Settings::sQueuedFrameCount]{ nullptr };

	UploadBuffer* mObjectCBuffer{ nullptr };
	D3D12_GPU_DESCRIPTOR_HANDLE mObjectCBufferGpuDescHandleBegin;

	D3D12_GPU_DESCRIPTOR_HANDLE mCubeMapBufferGpuDescHandleBegin;
};
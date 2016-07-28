#pragma once

#include <DirectXMath.h>

#include <RenderTask/CmdListRecorder.h>

class BasicTechCmdBuilderTask : public CmdListRecorder {
public:
	explicit BasicTechCmdBuilderTask(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);

	__forceinline UploadBuffer* &MaterialsBuffer() noexcept { return mMaterialsBuffer; }
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE& MaterialsGpuDescHandleBegin() noexcept { return mMaterialsGpuDescHandleBegin; }

	void RecordCommandLists(
		const DirectX::XMFLOAT4X4& view,
		const DirectX::XMFLOAT4X4& proj,
		const D3D12_CPU_DESCRIPTOR_HANDLE& backBufferHandle,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept override;	

protected:
	bool ValidateData() const noexcept;

private:
	D3D12_GPU_DESCRIPTOR_HANDLE mMaterialsGpuDescHandleBegin;
	UploadBuffer* mMaterialsBuffer{ nullptr };
};
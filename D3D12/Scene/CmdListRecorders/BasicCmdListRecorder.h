#pragma once

#include <DirectXMath.h>

#include <Scene/CmdListRecorder.h>

class BasicCmdListRecorder : public CmdListRecorder {
public:
	explicit BasicCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);

	__forceinline UploadBuffer* &MaterialsCBuffer() noexcept { return mMaterialsCBuffer; }
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE& MaterialsCBufferGpuDescHandleBegin() noexcept { return mMaterialsCBufferGpuDescHandleBegin; }

	void RecordCommandLists(
		const DirectX::XMFLOAT4X4& view,
		const DirectX::XMFLOAT4X4& proj,
		const D3D12_CPU_DESCRIPTOR_HANDLE* rtvCpuDescHandles,
		const std::uint32_t rtvCpuDescHandlesCount,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept override;	

protected:
	bool ValidateData() const noexcept;

private:
	D3D12_GPU_DESCRIPTOR_HANDLE mMaterialsCBufferGpuDescHandleBegin;
	UploadBuffer* mMaterialsCBuffer{ nullptr };
};
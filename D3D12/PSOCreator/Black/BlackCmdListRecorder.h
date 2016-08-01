#pragma once

#include <DirectXMath.h>

#include <RenderTask/CmdListRecorder.h>

class BlackCmdListRecorder : public CmdListRecorder {
public:
	explicit BlackCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);

	void RecordCommandLists(
		const DirectX::XMFLOAT4X4& view,
		const DirectX::XMFLOAT4X4& proj,
		const D3D12_CPU_DESCRIPTOR_HANDLE* geomPassRtvCpuDescHandles,
		const std::uint32_t geomPassRtvCpuDescHandlesCount,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept override;	
};
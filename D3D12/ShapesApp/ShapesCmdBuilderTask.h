#pragma once

#include <DirectXMath.h>

#include <RenderTask/CmdBuilderTask.h>

class ShapesCmdBuilderTask : public CmdBuilderTask {
public:
	explicit ShapesCmdBuilderTask(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);

	void BuildCommandLists(
		const DirectX::XMFLOAT4X4& view,
		const DirectX::XMFLOAT4X4& proj,
		const D3D12_CPU_DESCRIPTOR_HANDLE& backBufferHandle,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept override;	
};
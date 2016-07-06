#pragma once

#include <RenderTask/CmdBuilderTask.h>

class ShapesCmdBuilderTask : public CmdBuilderTask {
public:
	explicit ShapesCmdBuilderTask(ID3D12Device* device, const D3D12_VIEWPORT& screenViewport, const D3D12_RECT& scissorRect);

	void BuildCommandLists(
		tbb::concurrent_queue<ID3D12CommandList*>& cmdLists,
		const std::uint32_t currBackBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& backBufferHandle,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept override;	
};
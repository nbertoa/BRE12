#pragma once

#include <RenderTask/CmdBuilderTask.h>

class ShapeTask : public CmdBuilderTask {
public:
	explicit ShapeTask(ID3D12Device* device, const D3D12_VIEWPORT& screenViewport, const D3D12_RECT& scissorRect);

	void Execute(
		tbb::concurrent_queue<ID3D12CommandList*>& cmdLists,
		const std::uint32_t currBackBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& backBufferHandle,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept override;	
};
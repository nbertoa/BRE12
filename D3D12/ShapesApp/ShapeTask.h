#pragma once

#include <RenderTask/RenderTask.h>

class ShapeTask : public RenderTask {
public:
	explicit ShapeTask(const char* taskName, ID3D12Device* device, const D3D12_VIEWPORT& screenViewport, const D3D12_RECT& scissorRect);
	ShapeTask(const ShapeTask&) = delete;
	const ShapeTask& operator=(const ShapeTask&) = delete;

	void Init(const RenderTaskInitData& initData, tbb::concurrent_vector<ID3D12CommandList*>& cmdLists) noexcept override;
	void Update() noexcept override;
	void BuildCmdLists(
		tbb::concurrent_vector<ID3D12CommandList*>& cmdLists,
		const D3D12_CPU_DESCRIPTOR_HANDLE& backBufferHandle,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept override;
	
private:
	void BuildConstantBuffers() noexcept;
};
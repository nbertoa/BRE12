#pragma once

#include <RenderTask/InitTask.h>

class ShapeInitTask : public InitTask {
public:
	explicit ShapeInitTask(ID3D12Device* device, const D3D12_VIEWPORT& screenViewport, const D3D12_RECT& scissorRect);
	ShapeInitTask(const ShapeInitTask&) = delete;
	const ShapeInitTask& operator=(const ShapeInitTask&) = delete;

	void Init(const InitTaskInput& input, tbb::concurrent_vector<ID3D12CommandList*>& cmdLists, RenderTaskInput& output) noexcept override;

private:
	void BuildConstantBuffers(RenderTaskInput& output) noexcept;
};
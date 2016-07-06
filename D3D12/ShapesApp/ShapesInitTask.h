#pragma once

#include <RenderTask/InitTask.h>

class ShapesInitTask : public InitTask {
public:
	explicit ShapesInitTask() = default;

	void InitCmdBuilders(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdLists, CmdBuilderTaskInput& output) noexcept override;

private:
	void BuildConstantBuffers(ID3D12Device& device, CmdBuilderTaskInput& output) noexcept;
};
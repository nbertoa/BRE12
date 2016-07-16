#pragma once

#include <RenderTask/InitTask.h>

class ShapesInitTask : public InitTask {
public:
	explicit ShapesInitTask() = default;

	void InitCmdBuilders(tbb::concurrent_queue<ID3D12CommandList*>& cmdLists, CmdBuilderTaskInput& output) noexcept override;

private:
	void BuildConstantBuffers(CmdBuilderTaskInput& output) noexcept;
};
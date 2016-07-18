#pragma once

#include <memory>
#include <vector>
#include <RenderTask/CmdBuilderTask.h>

// You should inherit from this class and implement GenerateTasks method.
// Its generated CmdBuilderTask's are used by MasterRenderTask to 
// create/record command lists for rendering purposes.
// You should pass an instance to App constructor.
class Scene {
public:
	virtual void GenerateTasks(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue, std::vector<std::unique_ptr<CmdBuilderTask>>& tasks) const noexcept = 0;
};

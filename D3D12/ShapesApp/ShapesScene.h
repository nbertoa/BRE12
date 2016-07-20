#pragma once

#include <App/Scene.h>

class ShapesScene : public Scene {
public:	
	void GenerateTasks(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue, std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept override;
};

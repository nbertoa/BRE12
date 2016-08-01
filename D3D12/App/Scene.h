#pragma once

#include <memory>
#include <vector>
#include <RenderTask/CmdListRecorder.h>

// You should inherit from this class and implement needed methods.
// Its generated CmdListRecorder's are used by MasterRender to 
// create/record command lists for rendering purposes.
// You should pass an instance to App constructor.
class Scene {
public:
	Scene() = default;
	Scene(const Scene&) = delete;
	const Scene& operator=(const Scene&) = delete;

	virtual void GenerateGeomPassRecorders(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue, std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept = 0;
	virtual void GenerateLightPassRecorders(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue, std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept = 0;
	virtual void GeneratePostProcessingPassRecorders(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue, std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept = 0;
};

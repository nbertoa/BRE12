#pragma once

#include <App/Scene.h>

class ShapesScene : public Scene {
public:	
	void GenerateGeomPassRecorders(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue, std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept override;
	void GenerateLightPassRecorders(tbb::concurrent_queue<ID3D12CommandList*>& /*cmdListQueue*/, std::vector<std::unique_ptr<CmdListRecorder>>& /*tasks*/) const noexcept {}
	void GeneratePostProcessingPassRecorders(tbb::concurrent_queue<ID3D12CommandList*>& /*cmdListQueue*/, std::vector<std::unique_ptr<CmdListRecorder>>& /*tasks*/) const noexcept {}
};

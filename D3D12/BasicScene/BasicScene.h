#pragma once

#include <Scene/Scene.h>

class BasicScene : public Scene {
public:	
	void GenerateGeomPassRecorders(
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue, 
		CmdListHelper& cmdListHelper, 
		std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept override;
	
	void GenerateLightPassRecorders(
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept override;
	
	void GeneratePostProcessingPassRecorders(tbb::concurrent_queue<ID3D12CommandList*>& /*cmdListQueue*/, std::vector<std::unique_ptr<CmdListRecorder>>& /*tasks*/) const noexcept {}
};

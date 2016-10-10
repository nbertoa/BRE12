#pragma once

#include <Scene/Scene.h>

class NormalScene : public Scene {
public:
	void GenerateGeomPassRecorders(
		ID3D12CommandQueue& cmdQueue,
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>& tasks) noexcept override;

	void GenerateLightPassRecorders(
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		std::vector<std::unique_ptr<LightPassCmdListRecorder>>& tasks) noexcept override;

	void GenerateSkyBoxRecorder(
		ID3D12CommandQueue& cmdQueue,
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		std::unique_ptr<SkyBoxCmdListRecorder>& task) noexcept override;
};

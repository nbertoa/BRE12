#pragma once

#include <Scene/Scene.h>

class HeightScene : public Scene {
public:
	void GenerateGeomPassRecorders(
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		CmdListHelper& cmdListHelper,
		std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>& tasks) const noexcept override;

	void GenerateLightPassRecorders(
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		std::vector<std::unique_ptr<LightPassCmdListRecorder>>& tasks) const noexcept override;

	void GenerateSkyBoxRecorder(
		tbb::concurrent_queue<ID3D12CommandList*>& /*cmdListQueue*/,
		CmdListHelper& /*cmdListHelper*/,
		std::unique_ptr<SkyBoxCmdListRecorder>& /*task*/) const noexcept override {}
};

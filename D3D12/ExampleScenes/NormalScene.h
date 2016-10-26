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
		ID3D12Resource& depthBuffer,
		std::vector<std::unique_ptr<LightPassCmdListRecorder>>& tasks) noexcept override;

	void GenerateCubeMaps(
		ID3D12CommandQueue& cmdQueue,
		ID3D12Resource* &skyBoxCubeMap,
		ID3D12Resource* &diffuseIrradianceCubeMap,
		ID3D12Resource* &specularPreConvolvedCubeMap) noexcept override;
};

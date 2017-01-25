#pragma once

#include <Scene/Scene.h>

class HeightScene : public Scene {
public:
	HeightScene() = default;
	~HeightScene() = default;
	HeightScene(const HeightScene&) = delete;
	const HeightScene& operator=(const HeightScene&) = delete;
	HeightScene(HeightScene&&) = delete;
	HeightScene& operator=(HeightScene&&) = delete;

	void Init(ID3D12CommandQueue& cmdQueue) noexcept final override;

	void CreateGeometryPassRecorders(
		std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>& tasks) noexcept final override;

	void CreateLightingPassRecorders(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer,
		std::vector<std::unique_ptr<LightingPassCmdListRecorder>>& tasks) noexcept final override;

	void CreateCubeMapResources(
		ID3D12Resource* &skyBoxCubeMap,
		ID3D12Resource* &diffuseIrradianceCubeMap,
		ID3D12Resource* &specularPreConvolvedCubeMap) noexcept final override;
};

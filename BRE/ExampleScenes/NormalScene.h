#pragma once

#include <Scene/Scene.h>

class NormalScene : public Scene {
public:
	NormalScene() = default;
	~NormalScene() = default;
	NormalScene(const NormalScene&) = delete;
	const NormalScene& operator=(const NormalScene&) = delete;
	NormalScene(NormalScene&&) = delete;
	NormalScene& operator=(NormalScene&&) = delete;

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

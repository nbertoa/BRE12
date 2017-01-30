#pragma once

#include <Scene/Scene.h>

class MaterialShowcaseScene : public Scene {
public:
	MaterialShowcaseScene() = default;
	~MaterialShowcaseScene() = default;
	MaterialShowcaseScene(const MaterialShowcaseScene&) = delete;
	const MaterialShowcaseScene& operator=(const MaterialShowcaseScene&) = delete;
	MaterialShowcaseScene(MaterialShowcaseScene&&) = delete;
	MaterialShowcaseScene& operator=(MaterialShowcaseScene&&) = delete;

	void Init() noexcept final override;

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

#pragma once

#include <Scene/Scene.h>

class ColorHeightScene : public Scene {
public:
	ColorHeightScene() = default;
	~ColorHeightScene() = default;
	ColorHeightScene(const ColorHeightScene&) = delete;
	const ColorHeightScene& operator=(const ColorHeightScene&) = delete;
	ColorHeightScene(ColorHeightScene&&) = delete;
	ColorHeightScene& operator=(ColorHeightScene&&) = delete;

	void Init() noexcept final override;

	void CreateGeometryPassRecorders(
		std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>& tasks) noexcept final override;

	void CreateLightingPassRecorders(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer,
		std::vector<std::unique_ptr<LightingPassCmdListRecorder>>& tasks) noexcept final override;

	void CreateIndirectLightingResources(
		ID3D12Resource* &skyBoxCubeMap,
		ID3D12Resource* &diffuseIrradianceCubeMap,
		ID3D12Resource* &specularPreConvolvedCubeMap) noexcept final override;
};

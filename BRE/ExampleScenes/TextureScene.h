#pragma once

#include <Scene/Scene.h>

class TextureScene : public Scene {
public:	
	TextureScene() = default;
	~TextureScene() = default;
	TextureScene(const TextureScene&) = delete;
	const TextureScene& operator=(const TextureScene&) = delete;
	TextureScene(TextureScene&&) = delete;
	TextureScene& operator=(TextureScene&&) = delete;

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

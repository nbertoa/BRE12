#pragma once

#include <memory>
#include <vector>

#include <GeometryPass/GeometryPassCmdListRecorder.h>
#include <LightingPass/LightingPassCmdListRecorder.h>

struct ID3D12CommandAllocator;
struct ID3D12CommandQueue;
struct ID3D12Fence;
struct ID3D12GraphicsCommandList;

// Abstract class to inherit from.
// To generate CmdListRecorder's used by RenderManager
// to record command lists for rendering purposes
class Scene {
public:
	Scene() = default;
	virtual ~Scene() {}
	Scene(const Scene&) = delete;
	const Scene& operator=(const Scene&) = delete;
	Scene(Scene&&) = delete;
	Scene& operator=(Scene&&) = delete;

	virtual void Init() noexcept;
	
	// Preconditions:
	// - Init() must be called before
	virtual void CreateGeometryPassRecorders( 
		std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>& tasks) noexcept = 0;

	// Preconditions:
	// - Init() must be called before
	virtual void CreateLightingPassRecorders( 
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer,
		std::vector<std::unique_ptr<LightingPassCmdListRecorder>>& tasks) noexcept = 0;

	// Preconditions:
	// - Init() must be called before
	virtual void CreateCubeMapResources(
		ID3D12Resource* &skyBoxCubeMap,
		ID3D12Resource* &diffuseIrradianceCubeMap,
		ID3D12Resource* &specularPreConvolvedCubeMap) noexcept = 0;

protected:
	bool IsDataValid() const;

	ID3D12CommandAllocator* mCommandAllocators{ nullptr };
	ID3D12GraphicsCommandList* mCommandList{ nullptr };
};

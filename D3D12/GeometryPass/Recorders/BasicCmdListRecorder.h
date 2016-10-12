#pragma once

#include <GeometryPass/GeometryPassCmdListRecorder.h>

struct Material;

class BasicCmdListRecorder : public GeometryPassCmdListRecorder {
public:
	explicit BasicCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);

	// This method is to initialize PSO that is a shared between all this kind
	// of recorders.
	// This method is initialized by its corresponding pass.
	static void InitPSO(const DXGI_FORMAT* geometryBufferFormats, const std::uint32_t geometryBufferCount) noexcept;

	void Init(
		const GeometryData* geometryDataVec,
		const std::uint32_t numGeomData,
		const Material* materials,
		const std::uint32_t numMaterials,
		ID3D12Resource& cubeMap
		) noexcept;

	void RecordCommandLists(
		const FrameCBuffer& frameCBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE* rtvCpuDescHandles,
		const std::uint32_t rtvCpuDescHandlesCount,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept override;	

	bool ValidateData() const noexcept override;

private:
	void BuildBuffers(const Material* materials, const std::uint32_t numMaterials, ID3D12Resource& cubeMap) noexcept;

	D3D12_GPU_DESCRIPTOR_HANDLE mCubeMapBufferGpuDescHandleBegin;
};
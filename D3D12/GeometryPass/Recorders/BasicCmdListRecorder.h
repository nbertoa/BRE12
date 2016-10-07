#pragma once

#include <DirectXMath.h>

#include <GeometryPass/GeometryPassCmdListRecorder.h>

struct Material;

class BasicCmdListRecorder : public GeometryPassCmdListRecorder {
public:
	explicit BasicCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);

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

	bool ValidateData() const noexcept;

private:
	void BuildBuffers(const Material* materials, const std::uint32_t numMaterials, ID3D12Resource& cubeMap) noexcept;

	D3D12_GPU_DESCRIPTOR_HANDLE mCubeMapBufferGpuDescHandleBegin;
};
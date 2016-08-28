#pragma once

#include <DirectXMath.h>

#include <Scene/CmdListRecorder.h>

struct Material;

class NormalCmdListRecorder : public CmdListRecorder {
public:
	explicit NormalCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);

	void Init(
		const GeometryData* geometryDataVec,
		const std::uint32_t numGeomData,
		const Material* materials,
		ID3D12Resource** textures,
		ID3D12Resource** normals,
		const std::uint32_t numResources
	) noexcept;

	void RecordCommandLists(
		const DirectX::XMFLOAT4X4& view,
		const DirectX::XMFLOAT4X4& proj,
		const D3D12_CPU_DESCRIPTOR_HANDLE* rtvCpuDescHandles,
		const std::uint32_t rtvCpuDescHandlesCount,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept override;

	bool ValidateData() const noexcept;

private:
	void BuildBuffers(
		const Material* materials, 
		ID3D12Resource** textures, 
		ID3D12Resource** normals,
		const std::uint32_t dataCount) noexcept;

	std::vector<GeometryData> mGeometryDataVec;

	UploadBuffer* mFrameCBuffer[Settings::sQueuedFrameCount]{ nullptr };
	UploadBuffer* mImmutableCBuffer{ nullptr };

	UploadBuffer* mObjectCBuffer{ nullptr };
	D3D12_GPU_DESCRIPTOR_HANDLE mObjectCBufferGpuDescHandleBegin;

	D3D12_GPU_DESCRIPTOR_HANDLE mMaterialsCBufferGpuDescHandleBegin;
	UploadBuffer* mMaterialsCBuffer{ nullptr };

	D3D12_GPU_DESCRIPTOR_HANDLE mTexturesBufferGpuDescHandleBegin;
	D3D12_GPU_DESCRIPTOR_HANDLE mNormalsBufferGpuDescHandleBegin;
};
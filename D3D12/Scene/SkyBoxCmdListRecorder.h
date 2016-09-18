#pragma once

#include <cstdint>
#include <d3d12.h>
#include <DirectXMath.h>
#include <tbb/concurrent_queue.h>
#include <wrl.h>

#include <DXUtils/D3DFactory.h>
#include <GlobalData/Settings.h>
#include <ResourceManager/BufferCreator.h>

struct FrameCBuffer;
class UploadBuffer;

class SkyBoxCmdListRecorder {
public:
	struct GeometryData {
		GeometryData() = default;

		BufferCreator::VertexBufferData mVertexBufferData;
		BufferCreator::IndexBufferData mIndexBufferData;
		std::vector<DirectX::XMFLOAT4X4> mWorldMatrices;
	};

	explicit SkyBoxCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);

	void Init(const GeometryData& geometryData, ID3D12Resource& cubeMap) noexcept;

	void RecordCommandLists(
		const FrameCBuffer& frameCBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE* rtvCpuDescHandles,
		const std::uint32_t rtvCpuDescHandlesCount,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept;

	bool ValidateData() const noexcept;

private:
	void BuildBuffers(ID3D12Resource& cubeMap) noexcept;

	ID3D12Device& mDevice;
	tbb::concurrent_queue<ID3D12CommandList*>& mCmdListQueue;

	ID3D12GraphicsCommandList* mCmdList{ nullptr };
	ID3D12CommandAllocator* mCmdAlloc[Settings::sQueuedFrameCount]{ nullptr };
	std::uint32_t mCurrFrameIndex{ 0U };

	ID3D12DescriptorHeap* mCbvSrvUavDescHeap{ nullptr };
	ID3D12RootSignature* mRootSign{ nullptr };
	ID3D12PipelineState* mPSO{ nullptr };
	D3D12_PRIMITIVE_TOPOLOGY_TYPE mTopology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
	D3D12_VIEWPORT mScreenViewport{ 0.0f, 0.0f, (float)Settings::sWindowWidth, (float)Settings::sWindowHeight, 0.0f, 1.0f };
	D3D12_RECT mScissorRect{ 0, 0, Settings::sWindowWidth, Settings::sWindowHeight };

	GeometryData mGeometryData;

	UploadBuffer* mFrameCBuffer[Settings::sQueuedFrameCount]{ nullptr };

	UploadBuffer* mObjectCBuffer{ nullptr };
	D3D12_GPU_DESCRIPTOR_HANDLE mObjectCBufferGpuDescHandleBegin;

	D3D12_GPU_DESCRIPTOR_HANDLE mCubeMapBufferGpuDescHandleBegin;
};
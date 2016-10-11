#pragma once

#include <cstdint>
#include <d3d12.h>
#include <DirectXMath.h>
#include <tbb/concurrent_queue.h>
#include <wrl.h>

#include <DXUtils/D3DFactory.h>
#include <GlobalData/Settings.h>
#include <ResourceManager/BufferCreator.h>
#include <Utils/DebugUtils.h>

struct FrameCBuffer;
class UploadBuffer;

// Responsible of command lists recording to be executed by CommandListProcessor.
// This class has common data and functionality to record command lists for deferred shading geometry pass.
// Steps:
// - Inherit from it and reimplement RecordCommandLists() method
// - Call RecordCommandLists() to create command lists to execute in the GPU
class GeometryPassCmdListRecorder {
public:
	struct GeometryData {
		GeometryData() = default;

		BufferCreator::VertexBufferData mVertexBufferData;
		BufferCreator::IndexBufferData mIndexBufferData;
		std::vector<DirectX::XMFLOAT4X4> mWorldMatrices;
	};

	explicit GeometryPassCmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);

	// Record command lists and push them to the queue.
	virtual void RecordCommandLists(
		const FrameCBuffer& frameCBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept = 0;

	// This method validates all data (nullptr's, etc)
	// When you inherit from this class, you should reimplement it to include
	// new members
	virtual bool ValidateData() const noexcept;

protected:
	ID3D12Device& mDevice;
	tbb::concurrent_queue<ID3D12CommandList*>& mCmdListQueue;

	ID3D12GraphicsCommandList* mCmdList{ nullptr };
	ID3D12CommandAllocator* mCmdAlloc[Settings::sQueuedFrameCount]{ nullptr };
	std::uint32_t mCurrFrameIndex{ 0U };

	// Base command data. Once you inherits from this class, you should add
	// more class members that represent the extra information you need (like resources, for example)
	ID3D12DescriptorHeap* mCbvSrvUavDescHeap{ nullptr };
	D3D12_VIEWPORT mScreenViewport{ 0.0f, 0.0f, (float)Settings::sWindowWidth, (float)Settings::sWindowHeight, 0.0f, 1.0f };
	D3D12_RECT mScissorRect{ 0, 0, Settings::sWindowWidth, Settings::sWindowHeight };

	std::vector<GeometryData> mGeometryDataVec;

	UploadBuffer* mFrameCBuffer[Settings::sQueuedFrameCount]{ nullptr };
	UploadBuffer* mImmutableCBuffer{ nullptr };

	UploadBuffer* mObjectCBuffer{ nullptr };
	D3D12_GPU_DESCRIPTOR_HANDLE mObjectCBufferGpuDescHandleBegin;

	D3D12_GPU_DESCRIPTOR_HANDLE mMaterialsCBufferGpuDescHandleBegin;
	UploadBuffer* mMaterialsCBuffer{ nullptr };
};
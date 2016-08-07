#pragma once

#include <cstdint>
#include <d3d12.h>
#include <DirectXMath.h>
#include <tbb/concurrent_queue.h>
#include <wrl.h>

#include <DXUtils/D3DFactory.h>
#include <GlobalData/Settings.h>
#include <MathUtils/MathUtils.h>
#include <ResourceManager/BufferCreator.h>
#include <Utils/DebugUtils.h>

class UploadBuffer;

// Responsible of command lists recording to be executed by CommandListProcessor.
// Steps:
// - Inherit from CmdListRecorder and reimplement RecordCommandLists() method
// - Call CmdListRecorder::RecordCommandLists() to create command lists to execute in the GPU
class CmdListRecorder {
public:
	using VertexAndIndexBufferData = std::pair<BufferCreator::VertexBufferData, BufferCreator::IndexBufferData>;
	using VertexAndIndexBufferDataVec = std::vector<VertexAndIndexBufferData>;
	using Matrices = std::vector<DirectX::XMFLOAT4X4>;
	using MatricesByGeomIndex = std::vector<Matrices>;

	explicit CmdListRecorder(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);
		
	// Base command data. Once you inherits from this class, you should add
	// more class members that represent the extra information you need (like resources, for example)
	__forceinline ID3D12DescriptorHeap* & CbvSrvUavDescHeap() noexcept { return mCbvSrvUavDescHeap; }	
	__forceinline ID3D12RootSignature* &RootSign() noexcept { return mRootSign; }
	__forceinline ID3D12PipelineState* &PSO() noexcept { return mPSO; }
	__forceinline D3D12_PRIMITIVE_TOPOLOGY_TYPE& PrimitiveTopologyType() noexcept { return mTopology; }
	__forceinline UploadBuffer* &FrameCBuffer() noexcept { return mFrameCBuffer; }
	__forceinline UploadBuffer* &ObjectCBuffer() noexcept { return mObjectCBuffer; }
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE& ObjectCBufferGpuDescHandleBegin() noexcept { return mObjectCBufferGpuDescHandleBegin; }
	__forceinline D3D12_VIEWPORT& ScreenViewport() noexcept { return mScreenViewport; }
	__forceinline D3D12_RECT& ScissorRector() noexcept { return mScissorRect; }
	__forceinline VertexAndIndexBufferDataVec& GetVertexAndIndexBufferDataVec() noexcept { return mVertexAndIndexBufferDataVec; }
	__forceinline MatricesByGeomIndex& WorldMatricesByGeomIndex() noexcept { return mWorldMatricesByGeomIndex; }

	// Build command lists and push them to the queue.
	virtual void RecordCommandLists(
		const DirectX::XMFLOAT4X4& view,
		const DirectX::XMFLOAT4X4& proj,
		const D3D12_CPU_DESCRIPTOR_HANDLE* rtvCpuDescHandles,
		const std::uint32_t rtvCpuDescHandlesCount,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept = 0;

protected:
	// This method validates all data (nullptr's, etc)
	// When you inherit from this class, you should reimplement it to include
	// new members
	virtual bool ValidateData() const noexcept;

	ID3D12Device& mDevice;
	tbb::concurrent_queue<ID3D12CommandList*>& mCmdListQueue;

	ID3D12GraphicsCommandList* mCmdList{ nullptr };
	ID3D12CommandAllocator* mCmdAlloc[Settings::sQueuedFrameCount]{ nullptr };
	std::uint32_t mCurrCmdAllocIndex{ 0U };

	// Base command data. Once you inherits from this class, you should add
	// more class members that represent the extra information you need (like resources, for example)
	ID3D12DescriptorHeap* mCbvSrvUavDescHeap{ nullptr };
	ID3D12RootSignature* mRootSign{ nullptr };
	ID3D12PipelineState* mPSO{ nullptr };
	D3D12_PRIMITIVE_TOPOLOGY_TYPE mTopology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
	UploadBuffer* mFrameCBuffer{ nullptr };
	UploadBuffer* mObjectCBuffer{ nullptr };
	D3D12_VIEWPORT mScreenViewport{ 0.0f, 0.0f, (float)Settings::sWindowWidth, (float)Settings::sWindowHeight, 0.0f, 1.0f };
	D3D12_RECT mScissorRect{ 0, 0, Settings::sWindowWidth, Settings::sWindowHeight };
	
	// We should have a vector of world matrices per geometry.	
	VertexAndIndexBufferDataVec mVertexAndIndexBufferDataVec;
	MatricesByGeomIndex mWorldMatricesByGeomIndex;

	D3D12_GPU_DESCRIPTOR_HANDLE mObjectCBufferGpuDescHandleBegin;
};
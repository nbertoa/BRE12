#pragma once

#include <cstdint>
#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <tbb/concurrent_queue.h>
#include <wrl.h>

#include <DXUtils/D3DFactory.h>
#include <GeometryGenerator/GeometryGenerator.h>
#include <GlobalData/Settings.h>
#include <MathUtils/MathHelper.h>
#include <RenderTask/GeomBuffersCreator.h>
#include <Utils/DebugUtils.h>

class UploadBuffer;

struct GeometryData {
	GeometryData() = default;

	std::vector<DirectX::XMFLOAT4X4> mWorldMats;
	GeomBuffersCreator::Output mBuffersInfo;
};

// Task that is responsible of command lists recording to be executed by CommandListProcessor.
// Steps:
// - Inherit from CmdBuilderTask and reimplement BuildCommandLists() method
// - Call CmdBuilderTask::BuildCommandLists() to create command lists to execute in the GPU
class CmdBuilderTask {
public:
	using GeometryDataVec = std::vector<GeometryData>;

	explicit CmdBuilderTask(ID3D12Device& device, tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue);
		
	// Base command data. Once you inherits from this class, you should add
	// more class members that represent the extra information you need (like resources, for example)
	__forceinline ID3D12DescriptorHeap* & CVBHeap() noexcept { return mCBVHeap; }
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE& CBVBaseGpuDescHandle() noexcept { return mCbvBaseGpuDescHandle; }
	__forceinline ID3D12RootSignature* &RootSign() noexcept { return mRootSign; }
	__forceinline ID3D12PipelineState* &PSO() noexcept { return mPSO; }
	__forceinline D3D12_PRIMITIVE_TOPOLOGY_TYPE& PrimitiveTopologyType() noexcept { return mTopology; }
	__forceinline UploadBuffer* &FrameConstants() noexcept { return mFrameConstants; }
	__forceinline UploadBuffer* &ObjectConstants() noexcept { return mObjectConstants; }
	__forceinline D3D12_VIEWPORT& ScreenViewport() noexcept { return mScreenViewport; }
	__forceinline D3D12_RECT& ScissorRector() noexcept { return mScissorRect; }
	__forceinline GeometryDataVec& GeomDataVec() noexcept { return mGeomDataVec; }

	// Build command lists and push them to the queue.
	virtual void BuildCommandLists(
		const DirectX::XMFLOAT4X4& view,
		const DirectX::XMFLOAT4X4& proj,
		const D3D12_CPU_DESCRIPTOR_HANDLE& backBufferHandle,
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
	ID3D12DescriptorHeap* mCBVHeap{ nullptr };
	D3D12_GPU_DESCRIPTOR_HANDLE mCbvBaseGpuDescHandle;
	ID3D12RootSignature* mRootSign{ nullptr };
	ID3D12PipelineState* mPSO{ nullptr };
	D3D12_PRIMITIVE_TOPOLOGY_TYPE mTopology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
	UploadBuffer* mFrameConstants{ nullptr };
	UploadBuffer* mObjectConstants{ nullptr };
	D3D12_VIEWPORT mScreenViewport{ 0.0f, 0.0f, (float)Settings::sWindowWidth, (float)Settings::sWindowHeight, 0.0f, 1.0f };
	D3D12_RECT mScissorRect{ 0, 0, Settings::sWindowWidth, Settings::sWindowHeight };
	
	GeometryDataVec mGeomDataVec{};

private:
	void BuildCommandObjects() noexcept;
};
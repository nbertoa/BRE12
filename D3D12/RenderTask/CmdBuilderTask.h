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
#include <RenderTask/VertexIndexBufferCreatorTask.h>
#include <Utils/DebugUtils.h>

class UploadBuffer;

struct GeometryData {
	GeometryData() = default;

	DirectX::XMFLOAT4X4 mWorld{ MathHelper::Identity4x4() };

	VertexIndexBufferCreatorTask::Output mBuffersInfo;
};

struct CmdBuilderTaskInput {
	ID3D12DescriptorHeap* mCBVHeap{ nullptr };
	D3D12_GPU_DESCRIPTOR_HANDLE mCbvBaseGpuDescHandle;
	ID3D12RootSignature* mRootSign{ nullptr };
	ID3D12PipelineState* mPSO{ nullptr };

	ID3D12GraphicsCommandList* mCmdList{ nullptr };
	ID3D12CommandAllocator* mCmdAlloc[Settings::sQueuedFrameCount]{ nullptr };
	std::uint32_t mCurrCmdAllocIndex{ 0U };

	D3D12_PRIMITIVE_TOPOLOGY_TYPE mTopology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };

	using GeometryDataVec = std::vector<GeometryData>;
	GeometryDataVec mGeomDataVec{};

	UploadBuffer* mFrameConstants{ nullptr };
	UploadBuffer* mObjectConstants{ nullptr };
};

// Task that given its CmdBuilderTaskInput, creates command lists to be executed by CommandListProcessor.
// Steps:
// - Inherit from CmdBuilderTask and reimplement BuildCommandLists() method
// - Refer to InitTask documentation to know how to initialize CmdBuilderTaskInput (you should get it through CmdBuilderTask::TaskInput())
// - Call CmdBuilderTask::BuildCommandLists() to create command lists to execute in the GPU
class CmdBuilderTask {
public:
	explicit CmdBuilderTask(ID3D12Device* device, const D3D12_VIEWPORT& screenViewport, const D3D12_RECT& scissorRect);

	__forceinline CmdBuilderTaskInput& TaskInput() noexcept { return mInput; }
		
	// Build command lists and store them in the queue.
	virtual void BuildCommandLists(
		tbb::concurrent_queue<ID3D12CommandList*>& cmdLists,
		const DirectX::XMFLOAT4X4& view,
		const DirectX::XMFLOAT4X4& proj,
		const D3D12_CPU_DESCRIPTOR_HANDLE& backBufferHandle,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept = 0;

protected:
	ID3D12Device* mDevice{ nullptr };
	D3D12_VIEWPORT mViewport{};
	D3D12_RECT mScissorRect{};
	CmdBuilderTaskInput mInput;
};
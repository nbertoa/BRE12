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
#include <Utils/DebugUtils.h>

class UploadBuffer;

struct GeometryData {
	GeometryData() = default;

	DirectX::XMFLOAT4X4 mWorld{ MathHelper::Identity4x4() };

	ID3D12Resource* mVertexBuffer{ nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView{};

	ID3D12Resource* mIndexBuffer{ nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView{};

	std::uint32_t mIndexCount{ 0U };
	std::uint32_t mStartIndexLoc{ 0U };
	std::uint32_t mBaseVertexLoc{ 0U };
};

struct CmdBuilderTaskInput {
	ID3D12DescriptorHeap* mCBVHeap{ nullptr };
	ID3D12RootSignature* mRootSign{ nullptr };
	ID3D12PipelineState* mPSO{ nullptr };

	ID3D12GraphicsCommandList* mCmdList;
	ID3D12CommandAllocator* mCmdAlloc[Settings::sSwapChainBufferCount];

	D3D12_PRIMITIVE_TOPOLOGY_TYPE mTopology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };

	using GeometryDataVec = std::vector<GeometryData>;
	GeometryDataVec mGeomDataVec{};

	UploadBuffer* mFrameConstants{ nullptr };
	UploadBuffer* mObjectConstants{ nullptr };
};

// - Inherit from this class and reimplement virtual methods.
class CmdBuilderTask {
public:
	explicit CmdBuilderTask(ID3D12Device* device, const D3D12_VIEWPORT& screenViewport, const D3D12_RECT& scissorRect);

	__forceinline CmdBuilderTaskInput& TaskInput() noexcept { return mInput; }
		
	// Store new command lists in cmdLists  
	virtual void Execute(
		tbb::concurrent_queue<ID3D12CommandList*>& cmdLists,
		const std::uint32_t currBackBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& backBufferHandle,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthStencilHandle) noexcept = 0;

protected:
	ID3D12Device* mDevice{ nullptr };
	D3D12_VIEWPORT mViewport{};
	D3D12_RECT mScissorRect{};
	CmdBuilderTaskInput mInput;
};
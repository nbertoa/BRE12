#include "AmbientLightPass.h"

#include <d3d12.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <CommandManager\CommandManager.h>
#include <DXUtils\d3dx12.h>
#include <ModelManager\Mesh.h>
#include <ModelManager\Model.h>
#include <ModelManager\ModelManager.h>
#include <ResourceManager\ResourceManager.h>
#include <Utils\DebugUtils.h>

namespace {
	void CreateCommandObjects(
		ID3D12CommandAllocator* &cmdAlloc,
		ID3D12GraphicsCommandList* &cmdList,
		ID3D12Fence* &fence) noexcept {

		ASSERT(Settings::sQueuedFrameCount > 0U);
		ASSERT(cmdList == nullptr);

		// Create command allocator and command list
		CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc);
		CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAlloc, cmdList);
		cmdList->Close();

		ResourceManager::Get().CreateFence(0U, D3D12_FENCE_FLAG_NONE, fence);
	}

	void ExecuteCommandList(
		ID3D12CommandQueue& cmdQueue, 
		ID3D12GraphicsCommandList& cmdList, 
		ID3D12Fence& fence) noexcept {

		cmdList.Close();

		ID3D12CommandList* cmdLists[1U]{ &cmdList };
		cmdQueue.ExecuteCommandLists(_countof(cmdLists), cmdLists);

		const std::uint64_t fenceValue = fence.GetCompletedValue() + 1UL;

		CHECK_HR(cmdQueue.Signal(&fence, fenceValue));

		// Wait until the GPU has completed commands up to this fence point.
		if (fence.GetCompletedValue() < fenceValue) {
			const HANDLE eventHandle{ CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS) };
			ASSERT(eventHandle);

			// Fire event when GPU hits current fence.  
			CHECK_HR(fence.SetEventOnCompletion(fenceValue, eventHandle));

			// Wait until the GPU hits current fence event is fired.
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}

	void CreateAmbientAccessibilityBuffer(
		ID3D12Device& device,
		Microsoft::WRL::ComPtr<ID3D12Resource>& buffer,
		D3D12_CPU_DESCRIPTOR_HANDLE& bufferRTCpuDescHandle,
		ID3D12DescriptorHeap* &descHeap) noexcept {

		// Create buffer desc heap
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
		descHeapDesc.NumDescriptors = 1U;
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		descHeapDesc.NodeMask = 0;
		ResourceManager::Get().CreateDescriptorHeap(descHeapDesc, descHeap);

		// Set shared buffers properties
		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Alignment = 0U;
		resDesc.Width = Settings::sWindowWidth;
		resDesc.Height = Settings::sWindowHeight;
		resDesc.DepthOrArraySize = 1U;
		resDesc.MipLevels = 0U;
		resDesc.SampleDesc.Count = 1U;
		resDesc.SampleDesc.Quality = 0U;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_CLEAR_VALUE clearValue { DXGI_FORMAT_UNKNOWN, 0.0f, 0.0f, 0.0f, 0.0f };
		buffer.Reset();
		
		CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };

		ID3D12Resource* res{ nullptr };
		bufferRTCpuDescHandle = descHeap->GetCPUDescriptorHandleForHeapStart();
		const std::size_t rtvDescSize{ ResourceManager::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) };

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		// Create and store RTV's descriptor for buffer
		resDesc.Format = DXGI_FORMAT_R32_FLOAT;
		clearValue.Format = resDesc.Format;
		rtvDesc.Format = resDesc.Format;
		ResourceManager::Get().CreateCommittedResource(heapProps, D3D12_HEAP_FLAG_NONE, resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue, res);
		buffer = Microsoft::WRL::ComPtr<ID3D12Resource>(res);
		device.CreateRenderTargetView(buffer.Get(), &rtvDesc, bufferRTCpuDescHandle);
	}
}

void AmbientLightPass::Init(
	ID3D12Device& device,
	ID3D12CommandQueue& cmdQueue,
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	ID3D12Resource& baseColorMetalMaskBuffer,
	ID3D12Resource& normalSmoothnessBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
	ID3D12Resource& depthBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept {

	ASSERT(ValidateData() == false);
	
	CreateCommandObjects(mCmdAlloc, mCmdList, mFence);

	CHECK_HR(mCmdList->Reset(mCmdAlloc, nullptr));
	
	// Create model for a full screen quad geometry. 
	Model* model;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer;
	ModelManager::Get().CreateFullscreenQuad(model, *mCmdList, uploadVertexBuffer, uploadIndexBuffer);
	ASSERT(model != nullptr);

	// Get vertex and index buffers data from the only mesh this model must have.
	ASSERT(model->Meshes().size() == 1UL);
	const Mesh& mesh = model->Meshes()[0U];
	ExecuteCommandList(cmdQueue, *mCmdList, *mFence);

	// Initialize recorder's PSO
	AmbientCmdListRecorder::InitPSO();

	// Create ambient accessibility buffer
	CreateAmbientAccessibilityBuffer(
		device,
		mAmbientAccessibilityBuffer,
		mAmbientAccessibilityBufferRTCpuDescHandle,
		mDescHeap);

	// Initialize ambient light recorder
	mRecorder.reset(new AmbientCmdListRecorder(device, cmdListQueue));
	mRecorder->Init(
		mesh.VertexBufferData(), 
		mesh.IndexBufferData(), 
		baseColorMetalMaskBuffer,
		colorBufferCpuDesc,
		*mAmbientAccessibilityBuffer.Get(),
		mAmbientAccessibilityBufferRTCpuDescHandle,
		depthBufferCpuDesc);

	// Initialize ambient occlusion pass
	mAmbientOcclusionPass.Init(
		device, 
		cmdQueue, 
		cmdListQueue, 
		normalSmoothnessBuffer,
		mAmbientAccessibilityBufferRTCpuDescHandle,
		depthBuffer,
		depthBufferCpuDesc);

	ASSERT(ValidateData());
}

void AmbientLightPass::Execute() const noexcept {
	ASSERT(ValidateData());

	mRecorder->RecordAndPushCommandLists();
}

bool AmbientLightPass::ValidateData() const noexcept {
	const bool b =
		mCmdAlloc != nullptr &&
		mCmdList != nullptr &&
		mFence != nullptr &&
		mRecorder.get() != nullptr &&
		mAmbientAccessibilityBuffer.Get() != nullptr &&
		mAmbientAccessibilityBufferRTCpuDescHandle.ptr != 0UL &&
		mDescHeap != nullptr;

	return b;
}
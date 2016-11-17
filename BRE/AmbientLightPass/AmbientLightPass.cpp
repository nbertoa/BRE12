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
		ID3D12CommandAllocator* cmdAllocsBegin[Settings::sQueuedFrameCount],
		ID3D12CommandAllocator* cmdAllocsEnd[Settings::sQueuedFrameCount],
		ID3D12GraphicsCommandList* &cmdList,
		ID3D12Fence* &fence) noexcept {

		ASSERT(Settings::sQueuedFrameCount > 0U);
		ASSERT(cmdList == nullptr);

		// Create command allocators and command list
		for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
			ASSERT(cmdAllocsEnd[i] == nullptr);
			CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocsBegin[i]);

			ASSERT(cmdAllocsEnd[i] == nullptr);
			CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocsEnd[i]);
		}
		CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAllocsBegin[0], cmdList);
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

		D3D12_CLEAR_VALUE clearValue { DXGI_FORMAT_R24_UNORM_X8_TYPELESS, 0.0f, 0.0f, 0.0f, 0.0f };
		buffer.Reset();
		
		CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };

		ID3D12Resource* res{ nullptr };
		bufferRTCpuDescHandle = descHeap->GetCPUDescriptorHandleForHeapStart();
		const std::size_t rtvDescSize{ ResourceManager::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) };

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		// Create and store RTV's descriptor for buffer
		resDesc.Format = DXGI_FORMAT_R16_UNORM;
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

	mCmdQueue = &cmdQueue;
	
	CreateCommandObjects(mCmdAllocsBegin, mCmdAllocsEnd, mCmdList, mFence);

	CHECK_HR(mCmdList->Reset(mCmdAllocsBegin[0U], nullptr));
	
	// Create model for a full screen quad geometry. 
	Model* model;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer;
	ModelManager::Get().CreateFullscreenQuad(model, *mCmdList, uploadVertexBuffer, uploadIndexBuffer);
	ASSERT(model != nullptr);

	// Get vertex and index buffers data from the only mesh this model must have.
	ASSERT(model->Meshes().size() == 1UL);
	const Mesh& mesh = model->Meshes()[0U];
	ExecuteCommandList(*mCmdQueue, *mCmdList, *mFence);

	// Initialize recorder's PSO
	AmbientLightCmdListRecorder::InitPSO();
	AmbientOcclusionCmdListRecorder::InitPSO();

	// Create ambient accessibility buffer
	CreateAmbientAccessibilityBuffer(
		device,
		mAmbientAccessibilityBuffer,
		mAmbientAccessibilityBufferRTCpuDescHandle,
		mDescHeap);
	
	// Initialize ambient occlusion recorder
	mAmbientOcclusionRecorder.reset(new AmbientOcclusionCmdListRecorder(device, cmdListQueue));
	mAmbientOcclusionRecorder->Init(
		mesh.VertexBufferData(),
		mesh.IndexBufferData(),
		normalSmoothnessBuffer,
		mAmbientAccessibilityBufferRTCpuDescHandle,
		depthBuffer,
		depthBufferCpuDesc);

	// Initialize ambient light recorder
	mAmbientLightRecorder.reset(new AmbientLightCmdListRecorder(device, cmdListQueue));
	mAmbientLightRecorder->Init(
		mesh.VertexBufferData(), 
		mesh.IndexBufferData(), 
		baseColorMetalMaskBuffer,
		colorBufferCpuDesc,
		*mAmbientAccessibilityBuffer.Get(),
		mAmbientAccessibilityBufferRTCpuDescHandle,
		depthBufferCpuDesc);

	ASSERT(ValidateData());
}

void AmbientLightPass::Execute(const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(ValidateData());

	ExecuteBeginTask();
	mAmbientOcclusionRecorder->RecordAndPushCommandLists(frameCBuffer);
	ExecuteEndingTask();
	mAmbientLightRecorder->RecordAndPushCommandLists();		
}

bool AmbientLightPass::ValidateData() const noexcept {
	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		if (mCmdAllocsBegin[i] == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		if (mCmdAllocsEnd[i] == nullptr) {
			return false;
		}
	}

	const bool b =
		mCmdQueue != nullptr &&
		mCmdList != nullptr &&
		mFence != nullptr &&
		mAmbientOcclusionRecorder.get() != nullptr &&
		mAmbientLightRecorder.get() != nullptr &&
		mAmbientAccessibilityBuffer.Get() != nullptr &&
		mAmbientAccessibilityBufferRTCpuDescHandle.ptr != 0UL &&
		mDescHeap != nullptr;

	return b;
}

void AmbientLightPass::ExecuteBeginTask() noexcept {
	ASSERT(ValidateData());

	// Used to choose a different command list allocator each call.
	static std::uint32_t cmdAllocIndex{ 0U };

	ID3D12CommandAllocator* cmdAllocBegin{ mCmdAllocsBegin[cmdAllocIndex] };
	cmdAllocIndex = (cmdAllocIndex + 1U) % _countof(mCmdAllocsBegin);

	CHECK_HR(cmdAllocBegin->Reset());
	CHECK_HR(mCmdList->Reset(cmdAllocBegin, nullptr));

	// Resource barriers
	CD3DX12_RESOURCE_BARRIER barriers[]{
		CD3DX12_RESOURCE_BARRIER::Transition(mAmbientAccessibilityBuffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
	};
	const std::uint32_t barriersCount = _countof(barriers);
	ASSERT(barriersCount == 1UL);
	mCmdList->ResourceBarrier(barriersCount, barriers);

	// Clear render targets
	float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	mCmdList->ClearRenderTargetView(mAmbientAccessibilityBufferRTCpuDescHandle, clearColor, 0U, nullptr);
	CHECK_HR(mCmdList->Close());

	// Execute preliminary task
	{
		ID3D12CommandList* cmdLists[] = { mCmdList };
		mCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
	}
}

void AmbientLightPass::ExecuteEndingTask() noexcept {
	ASSERT(ValidateData());

	// Used to choose a different command list allocator each call.
	static std::uint32_t cmdAllocIndex{ 0U };

	ID3D12CommandAllocator* cmdAllocEnd{ mCmdAllocsEnd[cmdAllocIndex] };
	cmdAllocIndex = (cmdAllocIndex + 1U) % _countof(mCmdAllocsBegin);

	// Prepare end task
	CHECK_HR(cmdAllocEnd->Reset());
	CHECK_HR(mCmdList->Reset(cmdAllocEnd, nullptr));

	// Resource barriers
	CD3DX12_RESOURCE_BARRIER endBarriers[]{
		CD3DX12_RESOURCE_BARRIER::Transition(mAmbientAccessibilityBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
	};
	const std::uint32_t barriersCount = _countof(endBarriers);
	ASSERT(barriersCount == 1UL);
	mCmdList->ResourceBarrier(barriersCount, endBarriers);
	CHECK_HR(mCmdList->Close());

	// Execute end task
	{
		ID3D12CommandList* cmdLists[] = { mCmdList };
		mCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
	}
}
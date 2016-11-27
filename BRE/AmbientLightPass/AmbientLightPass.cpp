#include "AmbientLightPass.h"

#include <d3d12.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <CommandManager\CommandManager.h>
#include <DescriptorManager\DescriptorManager.h>
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
		ID3D12GraphicsCommandList* &cmdListBegin,
		ID3D12GraphicsCommandList* &cmdListEnd,
		ID3D12Fence* &fence) noexcept {

		ASSERT(Settings::sQueuedFrameCount > 0U);
		ASSERT(cmdListBegin == nullptr);
		ASSERT(cmdListEnd == nullptr);

		// Create command allocators and command list
		for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
			ASSERT(cmdAllocsEnd[i] == nullptr);
			CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocsBegin[i]);

			ASSERT(cmdAllocsEnd[i] == nullptr);
			CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocsEnd[i]);
		}

		CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAllocsBegin[0], cmdListBegin);
		cmdListBegin->Close();

		CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAllocsEnd[0], cmdListEnd);
		cmdListEnd->Close();

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
		Microsoft::WRL::ComPtr<ID3D12Resource>& buffer,
		D3D12_CPU_DESCRIPTOR_HANDLE& bufferRTCpuDescHandle) noexcept {
		
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
		resDesc.Format = DXGI_FORMAT_R16_UNORM;

		//D3D12_CLEAR_VALUE clearValue { DXGI_FORMAT_R24_UNORM_X8_TYPELESS, 0.0f, 0.0f, 0.0f, 0.0f };
		D3D12_CLEAR_VALUE clearValue{ resDesc.Format, 0.0f, 0.0f, 0.0f, 0.0f };
		buffer.Reset();
		
		CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };

		// Create buffer resource
		ID3D12Resource* res{ nullptr };			
		ResourceManager::Get().CreateCommittedResource(heapProps, D3D12_HEAP_FLAG_NONE, resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue, res);
		
		// Create RTV's descriptor for buffer
		buffer = Microsoft::WRL::ComPtr<ID3D12Resource>(res);
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Format = resDesc.Format;
		DescriptorManager::Get().CreateRenderTargetView(*buffer.Get(), rtvDesc, &bufferRTCpuDescHandle);
	}
}

void AmbientLightPass::Init(
	ID3D12Device& device,
	CommandListExecutor& cmdListExecutor,
	ID3D12CommandQueue& cmdQueue,
	ID3D12Resource& baseColorMetalMaskBuffer,
	ID3D12Resource& normalSmoothnessBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
	ID3D12Resource& depthBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept {

	ASSERT(ValidateData() == false);

	mCmdQueue = &cmdQueue;
	mCmdListExecutor = &cmdListExecutor;
	
	CreateCommandObjects(mCmdAllocsBegin, mCmdAllocsEnd, mCmdListBegin, mCmdListEnd, mFence);

	CHECK_HR(mCmdListBegin->Reset(mCmdAllocsBegin[0U], nullptr));
	
	// Create model for a full screen quad geometry. 
	Model* model;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadIndexBuffer;
	ModelManager::Get().CreateFullscreenQuad(model, *mCmdListBegin, uploadVertexBuffer, uploadIndexBuffer);
	ASSERT(model != nullptr);

	// Get vertex and index buffers data from the only mesh this model must have.
	ASSERT(model->Meshes().size() == 1UL);
	const Mesh& mesh = model->Meshes()[0U];
	ExecuteCommandList(*mCmdQueue, *mCmdListBegin, *mFence);

	// Initialize recorder's PSO
	AmbientLightCmdListRecorder::InitPSO();
	AmbientOcclusionCmdListRecorder::InitPSO();

	// Create ambient accessibility buffer
	CreateAmbientAccessibilityBuffer(mAmbientAccessibilityBuffer, mAmbientAccessibilityBufferRTCpuDescHandle);
	
	// Initialize ambient occlusion recorder
	mAmbientOcclusionRecorder.reset(new AmbientOcclusionCmdListRecorder(device, cmdListExecutor.CmdListQueue()));
	mAmbientOcclusionRecorder->Init(
		mesh.VertexBufferData(),
		mesh.IndexBufferData(),
		normalSmoothnessBuffer,
		mAmbientAccessibilityBufferRTCpuDescHandle,
		depthBuffer,
		depthBufferCpuDesc);

	// Initialize ambient light recorder
	mAmbientLightRecorder.reset(new AmbientLightCmdListRecorder(device, cmdListExecutor.CmdListQueue()));
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

void AmbientLightPass::Execute(const FrameCBuffer& /*frameCBuffer*/) noexcept {
	ASSERT(ValidateData());

	const std::uint32_t taskCount{ 3U };
	mCmdListExecutor->ResetExecutedCmdListCount();

	ExecuteBeginTask();
	//mAmbientOcclusionRecorder->RecordAndPushCommandLists(frameCBuffer);
	ExecuteEndingTask();
	mAmbientLightRecorder->RecordAndPushCommandLists();

	// Wait until all previous tasks command lists are executed
	while (mCmdListExecutor->ExecutedCmdListCount() < taskCount) {
		Sleep(0U);
	}
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
		mCmdListBegin != nullptr &&
		mCmdListEnd != nullptr &&
		mFence != nullptr &&
		mAmbientOcclusionRecorder.get() != nullptr &&
		mAmbientLightRecorder.get() != nullptr &&
		mAmbientAccessibilityBuffer.Get() != nullptr &&
		mAmbientAccessibilityBufferRTCpuDescHandle.ptr != 0UL &&
		mCmdListExecutor != nullptr;

	return b;
}

void AmbientLightPass::ExecuteBeginTask() noexcept {
	ASSERT(ValidateData());

	// Used to choose a different command list allocator each call.
	static std::uint32_t cmdAllocIndex{ 0U };

	ID3D12CommandAllocator* cmdAlloc{ mCmdAllocsBegin[cmdAllocIndex] };
	cmdAllocIndex = (cmdAllocIndex + 1U) % _countof(mCmdAllocsBegin);

	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCmdListBegin->Reset(cmdAlloc, nullptr));

	// Resource barriers
	CD3DX12_RESOURCE_BARRIER barriers[]{
		CD3DX12_RESOURCE_BARRIER::Transition(mAmbientAccessibilityBuffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
	};
	const std::uint32_t barriersCount = _countof(barriers);
	ASSERT(barriersCount == 1UL);
	mCmdListBegin->ResourceBarrier(barriersCount, barriers);

	// Clear render targets
	float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	mCmdListBegin->ClearRenderTargetView(mAmbientAccessibilityBufferRTCpuDescHandle, clearColor, 0U, nullptr);
	CHECK_HR(mCmdListBegin->Close());

	mCmdListExecutor->CmdListQueue().push(mCmdListBegin);
}

void AmbientLightPass::ExecuteEndingTask() noexcept {
	ASSERT(ValidateData());

	// Used to choose a different command list allocator each call.
	static std::uint32_t cmdAllocIndex{ 0U };

	ID3D12CommandAllocator* cmdAlloc{ mCmdAllocsEnd[cmdAllocIndex] };
	cmdAllocIndex = (cmdAllocIndex + 1U) % _countof(mCmdAllocsBegin);

	// Prepare end task
	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCmdListEnd->Reset(cmdAlloc, nullptr));

	// Resource barriers
	CD3DX12_RESOURCE_BARRIER endBarriers[]{
		CD3DX12_RESOURCE_BARRIER::Transition(mAmbientAccessibilityBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
	};
	const std::uint32_t barriersCount = _countof(endBarriers);
	ASSERT(barriersCount == 1UL);
	mCmdListEnd->ResourceBarrier(barriersCount, endBarriers);
	CHECK_HR(mCmdListEnd->Close());

	mCmdListExecutor->CmdListQueue().push(mCmdListEnd);
}
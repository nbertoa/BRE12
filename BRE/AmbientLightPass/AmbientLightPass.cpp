#include "AmbientLightPass.h"

#include <d3d12.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <CommandManager\CommandAllocatorManager.h>
#include <CommandManager\CommandListManager.h>
#include <DescriptorManager\RenderTargetDescriptorManager.h>
#include <DXUtils\d3dx12.h>
#include <ResourceManager\ResourceManager.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <Utils\DebugUtils.h>

namespace {
	void CreateCommandObjects(
		ID3D12CommandAllocator* cmdAllocsBegin[SettingsManager::sQueuedFrameCount],
		ID3D12CommandAllocator* cmdAllocsEnd[SettingsManager::sQueuedFrameCount],
		ID3D12GraphicsCommandList* &cmdListBegin,
		ID3D12GraphicsCommandList* &cmdListEnd) noexcept {

		ASSERT(SettingsManager::sQueuedFrameCount > 0U);
		ASSERT(cmdListBegin == nullptr);
		ASSERT(cmdListEnd == nullptr);

		// Create command allocators and command list
		for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
			ASSERT(cmdAllocsEnd[i] == nullptr);
			CommandAllocatorManager::Get().CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocsBegin[i]);

			ASSERT(cmdAllocsEnd[i] == nullptr);
			CommandAllocatorManager::Get().CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocsEnd[i]);
		}

		CommandListManager::Get().CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAllocsBegin[0], cmdListBegin);
		cmdListBegin->Close();

		CommandListManager::Get().CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAllocsEnd[0], cmdListEnd);
		cmdListEnd->Close();
	}

	void CreateBuffer(
		Microsoft::WRL::ComPtr<ID3D12Resource>& buffer,
		D3D12_CPU_DESCRIPTOR_HANDLE& bufferRTCpuDesc) noexcept {
		
		// Set shared buffers properties
		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Alignment = 0U;
		resDesc.Width = SettingsManager::sWindowWidth;
		resDesc.Height = SettingsManager::sWindowHeight;
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
		RenderTargetDescriptorManager::Get().CreateRenderTargetView(*buffer.Get(), rtvDesc, &bufferRTCpuDesc);
	}
}

void AmbientLightPass::Init(
	CommandListExecutor& cmdListExecutor,
	ID3D12CommandQueue& cmdQueue,
	ID3D12Resource& baseColorMetalMaskBuffer,
	ID3D12Resource& normalSmoothnessBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
	ID3D12Resource& depthBuffer) noexcept {

	ASSERT(ValidateData() == false);

	mCmdQueue = &cmdQueue;
	mCmdListExecutor = &cmdListExecutor;
	
	CreateCommandObjects(mCmdAllocsBegin, mCmdAllocsEnd, mCmdListBegin, mCmdListEnd);

	// Initialize recorder's PSO
	AmbientLightCmdListRecorder::InitPSO();
	AmbientOcclusionCmdListRecorder::InitPSO();
	BlurCmdListRecorder::InitPSO();

	// Create ambient accessibility buffer and blur buffer
	CreateBuffer(mAmbientAccessibilityBuffer, mAmbientAccessibilityBufferRTCpuDesc);
	CreateBuffer(mBlurBuffer, mBlurBufferRTCpuDesc);
	
	// Initialize ambient occlusion recorder
	mAmbientOcclusionRecorder.reset(new AmbientOcclusionCmdListRecorder(cmdListExecutor.CmdListQueue()));
	mAmbientOcclusionRecorder->Init(
		normalSmoothnessBuffer,
		mAmbientAccessibilityBufferRTCpuDesc,
		depthBuffer);

	// Initialize blur recorder
	mBlurRecorder.reset(new BlurCmdListRecorder(cmdListExecutor.CmdListQueue()));
	mBlurRecorder->Init(
		*mAmbientAccessibilityBuffer.Get(),
		mBlurBufferRTCpuDesc);

	// Initialize ambient light recorder
	mAmbientLightRecorder.reset(new AmbientLightCmdListRecorder(cmdListExecutor.CmdListQueue()));
	mAmbientLightRecorder->Init(
		baseColorMetalMaskBuffer,
		colorBufferCpuDesc,
		*mBlurBuffer.Get(),
		mBlurBufferRTCpuDesc);

	ASSERT(ValidateData());
}

void AmbientLightPass::Execute(const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(ValidateData());

	const std::uint32_t taskCount{ 5U };
	mCmdListExecutor->ResetExecutedCmdListCount();

	ExecuteBeginTask();
	mAmbientOcclusionRecorder->RecordAndPushCommandLists(frameCBuffer);
	mBlurRecorder->RecordAndPushCommandLists();
	ExecuteEndingTask();
	mAmbientLightRecorder->RecordAndPushCommandLists();

	// Wait until all previous tasks command lists are executed
	while (mCmdListExecutor->ExecutedCmdListCount() < taskCount) {
		Sleep(0U);
	}
}

bool AmbientLightPass::ValidateData() const noexcept {
	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCmdAllocsBegin[i] == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCmdAllocsEnd[i] == nullptr) {
			return false;
		}
	}

	const bool b =
		mCmdQueue != nullptr &&
		mCmdListBegin != nullptr &&
		mCmdListEnd != nullptr &&
		mAmbientOcclusionRecorder.get() != nullptr &&
		mAmbientLightRecorder.get() != nullptr &&
		mAmbientAccessibilityBuffer.Get() != nullptr &&
		mAmbientAccessibilityBufferRTCpuDesc.ptr != 0UL &&
		mBlurBuffer.Get() != nullptr &&
		mBlurBufferRTCpuDesc.ptr != 0UL &&
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
		ResourceStateManager::Get().TransitionState(*mAmbientAccessibilityBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET),
		ResourceStateManager::Get().TransitionState(*mBlurBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET),		
	};
	const std::uint32_t barriersCount = _countof(barriers);
	ASSERT(barriersCount == 2UL);
	mCmdListBegin->ResourceBarrier(barriersCount, barriers);

	// Clear render targets
	float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	mCmdListBegin->ClearRenderTargetView(mAmbientAccessibilityBufferRTCpuDesc, clearColor, 0U, nullptr);
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
		ResourceStateManager::Get().TransitionState(*mAmbientAccessibilityBuffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		ResourceStateManager::Get().TransitionState(*mBlurBuffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),		
	};
	const std::uint32_t barriersCount = _countof(endBarriers);
	ASSERT(barriersCount == 2UL);
	mCmdListEnd->ResourceBarrier(barriersCount, endBarriers);
	CHECK_HR(mCmdListEnd->Close());

	mCmdListExecutor->CmdListQueue().push(mCmdListEnd);
}
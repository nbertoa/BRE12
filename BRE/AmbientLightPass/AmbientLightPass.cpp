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
		ID3D12CommandAllocator* cmdAllocatorsBegin[SettingsManager::sQueuedFrameCount],
		ID3D12CommandAllocator* cmdAllocatorsEnd[SettingsManager::sQueuedFrameCount],
		ID3D12GraphicsCommandList* &commandListBegin,
		ID3D12GraphicsCommandList* &commandListEnd) noexcept 
	{
		ASSERT(SettingsManager::sQueuedFrameCount > 0U);
		ASSERT(commandListBegin == nullptr);
		ASSERT(commandListEnd == nullptr);

		// Create command allocators and command list
		for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
			ASSERT(cmdAllocatorsEnd[i] == nullptr);
			CommandAllocatorManager::Get().CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocatorsBegin[i]);

			ASSERT(cmdAllocatorsEnd[i] == nullptr);
			CommandAllocatorManager::Get().CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocatorsEnd[i]);
		}

		CommandListManager::Get().CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAllocatorsBegin[0], commandListBegin);
		commandListBegin->Close();

		CommandListManager::Get().CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAllocatorsEnd[0], commandListEnd);
		commandListEnd->Close();
	}

	void CreateResourceAndRenderTargetDescriptor(
		Microsoft::WRL::ComPtr<ID3D12Resource>& buffer,
		D3D12_CPU_DESCRIPTOR_HANDLE& bufferRenderTargetCpuDesc) noexcept {
		
		// Set shared buffers properties
		D3D12_RESOURCE_DESC resourceDescriptor = {};
		resourceDescriptor.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDescriptor.Alignment = 0U;
		resourceDescriptor.Width = SettingsManager::sWindowWidth;
		resourceDescriptor.Height = SettingsManager::sWindowHeight;
		resourceDescriptor.DepthOrArraySize = 1U;
		resourceDescriptor.MipLevels = 0U;
		resourceDescriptor.SampleDesc.Count = 1U;
		resourceDescriptor.SampleDesc.Quality = 0U;
		resourceDescriptor.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDescriptor.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		resourceDescriptor.Format = DXGI_FORMAT_R16_UNORM;

		D3D12_CLEAR_VALUE clearValue{ resourceDescriptor.Format, 0.0f, 0.0f, 0.0f, 0.0f };
		buffer.Reset();
		
		CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };

		// Create buffer resource
		ID3D12Resource* resource{ nullptr };			
		ResourceManager::Get().CreateCommittedResource(
			heapProps, 
			D3D12_HEAP_FLAG_NONE, 
			resourceDescriptor, 
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 
			&clearValue, 
			resource);
		
		// Create RTV's descriptor for buffer
		buffer = Microsoft::WRL::ComPtr<ID3D12Resource>(resource);
		D3D12_RENDER_TARGET_VIEW_DESC rtvDescriptor{};
		rtvDescriptor.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDescriptor.Format = resourceDescriptor.Format;
		RenderTargetDescriptorManager::Get().CreateRenderTargetView(*buffer.Get(), rtvDescriptor, &bufferRenderTargetCpuDesc);
	}
}

void AmbientLightPass::Init(
	ID3D12CommandQueue& commandQueue,
	ID3D12Resource& baseColorMetalMaskBuffer,
	ID3D12Resource& normalSmoothnessBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
	ID3D12Resource& depthBuffer) noexcept {

	ASSERT(ValidateData() == false);

	mCommandQueue = &commandQueue;
	
	CreateCommandObjects(mCmdAllocatorsBegin, mCmdAllocatorsEnd, mCmdListBegin, mCmdListEnd);

	// Initialize recorder's PSO
	AmbientLightCmdListRecorder::InitPSO();
	AmbientOcclusionCmdListRecorder::InitPSO();
	BlurCmdListRecorder::InitPSO();

	// Create ambient accessibility buffer and blur buffer
	CreateResourceAndRenderTargetDescriptor(mAmbientAccessibilityBuffer, mAmbientAccessibilityBufferRenderTargetCpuDesc);
	CreateResourceAndRenderTargetDescriptor(mBlurBuffer, mBlurBufferRenderTargetCpuDesc);
	
	// Initialize ambient occlusion recorder
	mAmbientOcclusionRecorder.reset(new AmbientOcclusionCmdListRecorder());
	mAmbientOcclusionRecorder->Init(
		normalSmoothnessBuffer,
		mAmbientAccessibilityBufferRenderTargetCpuDesc,
		depthBuffer);

	// Initialize blur recorder
	mBlurRecorder.reset(new BlurCmdListRecorder());
	mBlurRecorder->Init(
		*mAmbientAccessibilityBuffer.Get(),
		mBlurBufferRenderTargetCpuDesc);

	// Initialize ambient light recorder
	mAmbientLightRecorder.reset(new AmbientLightCmdListRecorder());
	mAmbientLightRecorder->Init(
		baseColorMetalMaskBuffer,
		colorBufferCpuDesc,
		*mBlurBuffer.Get(),
		mBlurBufferRenderTargetCpuDesc);

	ASSERT(ValidateData());
}

void AmbientLightPass::Execute(const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(ValidateData());

	const std::uint32_t taskCount{ 5U };
	CommandListExecutor::Get().ResetExecutedCommandListCount();

	ExecuteBeginTask();
	mAmbientOcclusionRecorder->RecordAndPushCommandLists(frameCBuffer);
	mBlurRecorder->RecordAndPushCommandLists();
	ExecuteEndingTask();
	mAmbientLightRecorder->RecordAndPushCommandLists();

	// Wait until all previous tasks command lists are executed
	while (CommandListExecutor::Get().GetExecutedCommandListCount() < taskCount) {
		Sleep(0U);
	}
}

bool AmbientLightPass::ValidateData() const noexcept {
	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCmdAllocatorsBegin[i] == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCmdAllocatorsEnd[i] == nullptr) {
			return false;
		}
	}

	const bool b =
		mCommandQueue != nullptr &&
		mCmdListBegin != nullptr &&
		mCmdListEnd != nullptr &&
		mAmbientOcclusionRecorder.get() != nullptr &&
		mAmbientLightRecorder.get() != nullptr &&
		mAmbientAccessibilityBuffer.Get() != nullptr &&
		mAmbientAccessibilityBufferRenderTargetCpuDesc.ptr != 0UL &&
		mBlurBuffer.Get() != nullptr &&
		mBlurBufferRenderTargetCpuDesc.ptr != 0UL;

	return b;
}

void AmbientLightPass::ExecuteBeginTask() noexcept {
	ASSERT(ValidateData());

	// Used to choose a different command list allocator each call.
	static std::uint32_t commandAllocatorIndex{ 0U };

	ID3D12CommandAllocator* commandAllocator{ mCmdAllocatorsBegin[commandAllocatorIndex] };
	commandAllocatorIndex = (commandAllocatorIndex + 1U) % _countof(mCmdAllocatorsBegin);

	CHECK_HR(commandAllocator->Reset());
	CHECK_HR(mCmdListBegin->Reset(commandAllocator, nullptr));

	// GetResource barriers
	CD3DX12_RESOURCE_BARRIER barriers[]{
		ResourceStateManager::Get().ChangeResourceStateAndGetBarrier(*mAmbientAccessibilityBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET),
		ResourceStateManager::Get().ChangeResourceStateAndGetBarrier(*mBlurBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET),		
	};
	const std::uint32_t barriersCount = _countof(barriers);
	ASSERT(barriersCount == 2UL);
	mCmdListBegin->ResourceBarrier(barriersCount, barriers);

	// Clear render targets
	float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	mCmdListBegin->ClearRenderTargetView(mAmbientAccessibilityBufferRenderTargetCpuDesc, clearColor, 0U, nullptr);
	CHECK_HR(mCmdListBegin->Close());

	CommandListExecutor::Get().AddCommandList(*mCmdListBegin);
}

void AmbientLightPass::ExecuteEndingTask() noexcept {
	ASSERT(ValidateData());

	// Used to choose a different command list allocator each call.
	static std::uint32_t commandAllocatorIndex{ 0U };

	ID3D12CommandAllocator* commandAllocator{ mCmdAllocatorsEnd[commandAllocatorIndex] };
	commandAllocatorIndex = (commandAllocatorIndex + 1U) % _countof(mCmdAllocatorsBegin);

	// Prepare end task
	CHECK_HR(commandAllocator->Reset());
	CHECK_HR(mCmdListEnd->Reset(commandAllocator, nullptr));

	// GetResource barriers
	CD3DX12_RESOURCE_BARRIER endBarriers[]{
		ResourceStateManager::Get().ChangeResourceStateAndGetBarrier(*mAmbientAccessibilityBuffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		ResourceStateManager::Get().ChangeResourceStateAndGetBarrier(*mBlurBuffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),		
	};
	const std::uint32_t barriersCount = _countof(endBarriers);
	ASSERT(barriersCount == 2UL);
	mCmdListEnd->ResourceBarrier(barriersCount, endBarriers);
	CHECK_HR(mCmdListEnd->Close());

	CommandListExecutor::Get().AddCommandList(*mCmdListEnd);
}
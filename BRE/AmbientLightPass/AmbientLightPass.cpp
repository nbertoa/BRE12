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
		ID3D12CommandAllocator* commandAllocators[SettingsManager::sQueuedFrameCount],
		ID3D12GraphicsCommandList* &commandList) noexcept 
	{
		ASSERT(SettingsManager::sQueuedFrameCount > 0U);
		ASSERT(commandList == nullptr);

#ifdef _DEBUG
		for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
			ASSERT(commandAllocators[i] == nullptr);
		}
#endif

		// Create command allocators and command list
		for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
			ASSERT(commandAllocators[i] == nullptr);
			commandAllocators[i] = &CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
		}

		commandList = &CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *commandAllocators[0]);
		commandList->Close();
	}

	void CreateResourceAndRenderTargetDescriptor(
		Microsoft::WRL::ComPtr<ID3D12Resource>& resource,
		D3D12_CPU_DESCRIPTOR_HANDLE& resourceRenderTargetCpuDesc,
		const D3D12_RESOURCE_STATES& resourceStates,
		const wchar_t* resourceName) noexcept 
	{		
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
		resource.Reset();
		
		CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };

		// Create buffer resource
		ID3D12Resource* resourcePtr = &ResourceManager::CreateCommittedResource(
			heapProps, 
			D3D12_HEAP_FLAG_NONE, 
			resourceDescriptor, 
			resourceStates,
			&clearValue,
			resourceName);
		
		// Create RTV's descriptor for buffer
		resource = Microsoft::WRL::ComPtr<ID3D12Resource>(resourcePtr);
		D3D12_RENDER_TARGET_VIEW_DESC rtvDescriptor{};
		rtvDescriptor.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDescriptor.Format = resourceDescriptor.Format;
		RenderTargetDescriptorManager::CreateRenderTargetView(*resource.Get(), rtvDescriptor, &resourceRenderTargetCpuDesc);
	}
}

void AmbientLightPass::Init(
	ID3D12Resource& baseColorMetalMaskBuffer,
	ID3D12Resource& normalSmoothnessBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
	ID3D12Resource& depthBuffer) noexcept 
{
	ASSERT(ValidateData() == false);
		
	CreateCommandObjects(mCommandAllocatorsBegin, mCommandListBegin);
	CreateCommandObjects(mCommandAllocatorsMiddle, mCommandListMiddle);
	CreateCommandObjects(mCommandAllocatorsFinal, mCommandListEnd);

	// Initialize recorder's PSO
	AmbientLightCmdListRecorder::InitPSO();
	AmbientOcclusionCmdListRecorder::InitPSO();
	BlurCmdListRecorder::InitPSO();

	// Create ambient accessibility buffer and blur buffer
	CreateResourceAndRenderTargetDescriptor(
		mAmbientAccessibilityBuffer, 
		mAmbientAccessibilityBufferRenderTargetCpuDescriptor,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		L"Ambient Accessibility Buffer");
	CreateResourceAndRenderTargetDescriptor(
		mBlurBuffer, 
		mBlurBufferRenderTargetCpuDescriptor,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		L"Blur Buffer");
	
	// Initialize ambient occlusion recorder
	mAmbientOcclusionRecorder.reset(new AmbientOcclusionCmdListRecorder());
	mAmbientOcclusionRecorder->Init(
		normalSmoothnessBuffer,
		mAmbientAccessibilityBufferRenderTargetCpuDescriptor,
		depthBuffer);

	// Initialize blur recorder
	mBlurRecorder.reset(new BlurCmdListRecorder());
	mBlurRecorder->Init(
		*mAmbientAccessibilityBuffer.Get(),
		mBlurBufferRenderTargetCpuDescriptor);

	// Initialize ambient light recorder
	mAmbientLightRecorder.reset(new AmbientLightCmdListRecorder());
	mAmbientLightRecorder->Init(
		baseColorMetalMaskBuffer,
		colorBufferCpuDesc,
		*mBlurBuffer.Get(),
		mBlurBufferRenderTargetCpuDescriptor);

	ASSERT(ValidateData());
}

void AmbientLightPass::Execute(const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(ValidateData());

	const std::uint32_t taskCount{ 5U };
	CommandListExecutor::Get().ResetExecutedCommandListCount();

	ExecuteBeginTask();
	mAmbientOcclusionRecorder->RecordAndPushCommandLists(frameCBuffer);
	ExecuteMiddleTask();
	mBlurRecorder->RecordAndPushCommandLists();
	ExecuteFinalTask();
	mAmbientLightRecorder->RecordAndPushCommandLists();

	// Wait until all previous tasks command lists are executed
	while (CommandListExecutor::Get().GetExecutedCommandListCount() < taskCount) {
		Sleep(0U);
	}
}

bool AmbientLightPass::ValidateData() const noexcept {
	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCommandAllocatorsBegin[i] == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCommandAllocatorsMiddle[i] == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCommandAllocatorsFinal[i] == nullptr) {
			return false;
		}
	}

	const bool b =
		mCommandListBegin != nullptr &&
		mCommandListMiddle != nullptr &&
		mCommandListEnd != nullptr &&
		mAmbientOcclusionRecorder.get() != nullptr &&
		mAmbientLightRecorder.get() != nullptr &&
		mAmbientAccessibilityBuffer.Get() != nullptr &&
		mAmbientAccessibilityBufferRenderTargetCpuDescriptor.ptr != 0UL &&
		mBlurBuffer.Get() != nullptr &&
		mBlurBufferRenderTargetCpuDescriptor.ptr != 0UL;

	return b;
}

void AmbientLightPass::ExecuteBeginTask() noexcept {
	ASSERT(ValidateData());

	// Check resource states:
	// Ambient accesibility buffer was used as pixel shader resource by blur shader.
	ASSERT(ResourceStateManager::GetResourceState(*mAmbientAccessibilityBuffer.Get()) == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// Blur buffer was used as pixel shader resource by ambient light shader
	ASSERT(ResourceStateManager::GetResourceState(*mBlurBuffer.Get()) == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// Used to choose a different command list allocator each call.
	static std::uint32_t commandAllocatorIndex{ 0U };

	ID3D12CommandAllocator* commandAllocator{ mCommandAllocatorsBegin[commandAllocatorIndex] };
	commandAllocatorIndex = (commandAllocatorIndex + 1U) % _countof(mCommandAllocatorsBegin);

	CHECK_HR(commandAllocator->Reset());
	CHECK_HR(mCommandListBegin->Reset(commandAllocator, nullptr));

	// Resource barriers
	CD3DX12_RESOURCE_BARRIER barriers[]
	{
		ResourceStateManager::ChangeResourceStateAndGetBarrier(
			*mAmbientAccessibilityBuffer.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET),
	};
	const std::uint32_t barriersCount = _countof(barriers);
	ASSERT(barriersCount == 1UL);
	mCommandListBegin->ResourceBarrier(barriersCount, barriers);

	float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	mCommandListBegin->ClearRenderTargetView(mAmbientAccessibilityBufferRenderTargetCpuDescriptor, clearColor, 0U, nullptr);

	CHECK_HR(mCommandListBegin->Close());
	CommandListExecutor::Get().AddCommandList(*mCommandListBegin);
}

void AmbientLightPass::ExecuteMiddleTask() noexcept {
	ASSERT(ValidateData());

	// Check resource states:
	// Ambient accesibility buffer was used as render target resource by ambient accesibility shader.
	ASSERT(ResourceStateManager::GetResourceState(*mAmbientAccessibilityBuffer.Get()) == D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Blur buffer was used as pixel shader resource by ambient light shader
	ASSERT(ResourceStateManager::GetResourceState(*mBlurBuffer.Get()) == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// Check resource states:
	// Ambient accesibility was used as pixel shader resource and must be changed to 
	ASSERT(ResourceStateManager::GetResourceState(*mAmbientAccessibilityBuffer.Get()) == D3D12_RESOURCE_STATE_RENDER_TARGET);

	ASSERT(ResourceStateManager::GetResourceState(*mBlurBuffer.Get()) == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// Used to choose a different command list allocator each call.
	static std::uint32_t commandAllocatorIndex{ 0U };

	ID3D12CommandAllocator* commandAllocator{ mCommandAllocatorsMiddle[commandAllocatorIndex] };
	commandAllocatorIndex = (commandAllocatorIndex + 1U) % _countof(mCommandAllocatorsMiddle);

	// Prepare end task
	CHECK_HR(commandAllocator->Reset());
	CHECK_HR(mCommandListMiddle->Reset(commandAllocator, nullptr));

	// Resource barriers
	CD3DX12_RESOURCE_BARRIER barriers[]
	{
		ResourceStateManager::ChangeResourceStateAndGetBarrier(
			*mAmbientAccessibilityBuffer.Get(), 
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),

		ResourceStateManager::ChangeResourceStateAndGetBarrier(
			*mBlurBuffer.Get(), 
			D3D12_RESOURCE_STATE_RENDER_TARGET),
	};
	const std::uint32_t barriersCount = _countof(barriers);
	ASSERT(barriersCount == 2UL);
	mCommandListMiddle->ResourceBarrier(barriersCount, barriers);

	CHECK_HR(mCommandListMiddle->Close());
	CommandListExecutor::Get().AddCommandList(*mCommandListMiddle);
}

void AmbientLightPass::ExecuteFinalTask() noexcept {
	ASSERT(ValidateData());

	// Check resource states:
	// Ambient accesibility buffer was used as pixel shader resource by blur shader.
	ASSERT(ResourceStateManager::GetResourceState(*mAmbientAccessibilityBuffer.Get()) == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// Blur buffer was used as render target resource by blur shader
	ASSERT(ResourceStateManager::GetResourceState(*mBlurBuffer.Get()) == D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Used to choose a different command list allocator each call.
	static std::uint32_t commandAllocatorIndex{ 0U };

	ID3D12CommandAllocator* commandAllocator{ mCommandAllocatorsFinal[commandAllocatorIndex] };
	commandAllocatorIndex = (commandAllocatorIndex + 1U) % _countof(mCommandAllocatorsBegin);

	// Prepare end task
	CHECK_HR(commandAllocator->Reset());
	CHECK_HR(mCommandListEnd->Reset(commandAllocator, nullptr));
	
	// Resource barriers
	CD3DX12_RESOURCE_BARRIER barriers[]
	{
		ResourceStateManager::ChangeResourceStateAndGetBarrier(*mBlurBuffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
	};
	const std::uint32_t barriersCount = _countof(barriers);
	ASSERT(barriersCount == 1UL);
	mCommandListEnd->ResourceBarrier(barriersCount, barriers);

	CHECK_HR(mCommandListEnd->Close());
	CommandListExecutor::Get().AddCommandList(*mCommandListEnd);
}
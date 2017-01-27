#include "PostProcessPass.h"

#include <d3d12.h>
#include <DirectXColors.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <CommandManager\CommandAllocatorManager.h>
#include <CommandManager\CommandListManager.h>
#include <DXUtils/d3dx12.h>
#include <ResourceManager\ResourceManager.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils\DebugUtils.h>

namespace {
	void CreateCommandObjects(
		ID3D12CommandAllocator* cmdAllocators[SettingsManager::sQueuedFrameCount],
		ID3D12GraphicsCommandList* &commandList) noexcept {

		ASSERT(SettingsManager::sQueuedFrameCount > 0U);
		ASSERT(commandList == nullptr);

		// Create command allocators and command list
		for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
			ASSERT(cmdAllocators[i] == nullptr);
			CommandAllocatorManager::Get().CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocators[i]);
		}
		CommandListManager::Get().CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAllocators[0], commandList);
		commandList->Close();
	}
}

void PostProcessPass::Init(
	ID3D12CommandQueue& commandQueue,
	ID3D12Resource& colorBuffer) noexcept {

	ASSERT(IsDataValid() == false);
	
	CreateCommandObjects(mCommandAllocators, mCommandList);
	mCommandQueue = &commandQueue;
	mColorBuffer = &colorBuffer;

	// Initialize recorder's PSO
	PostProcessCmdListRecorder::InitPSO();

	// Initialize recorder
	mRecorder.reset(new PostProcessCmdListRecorder());
	mRecorder->Init(colorBuffer);

	ASSERT(IsDataValid());
}

void PostProcessPass::Execute(
	ID3D12Resource& frameBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& frameBufferCpuDesc) noexcept {

	ASSERT(IsDataValid());
	ASSERT(frameBufferCpuDesc.ptr != 0UL);

	ExecuteBeginTask(frameBuffer, frameBufferCpuDesc);

	CommandListExecutor::Get().ResetExecutedCommandListCount();
	mRecorder->RecordAndPushCommandLists(frameBufferCpuDesc);
	
	// Wait until all previous tasks command lists are executed
	while (CommandListExecutor::Get().GetExecutedCommandListCount() < 1) {
		Sleep(0U);
	}
}

bool PostProcessPass::IsDataValid() const noexcept {
	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCommandAllocators[i] == nullptr) {
			return false;
		}
	}

	const bool b =
		mCommandQueue != nullptr &&
		mCommandList != nullptr &&
		mRecorder.get() != nullptr &&
		mColorBuffer != nullptr;

	return b;
}

void PostProcessPass::ExecuteBeginTask(
	ID3D12Resource& frameBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& frameBufferCpuDesc) noexcept {

	ASSERT(IsDataValid());
	ASSERT(frameBufferCpuDesc.ptr != 0UL);

	// Used to choose a different command list allocator each call.
	static std::uint32_t cmdAllocIndex{ 0U };

	ID3D12CommandAllocator* cmdAlloc{ mCommandAllocators[cmdAllocIndex] };
	cmdAllocIndex = (cmdAllocIndex + 1U) % _countof(mCommandAllocators);

	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCommandList->Reset(cmdAlloc, nullptr));

	// Set barriers
	CD3DX12_RESOURCE_BARRIER barriers[]{
		ResourceStateManager::Get().ChangeResourceStateAndGetBarrier(*mColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		ResourceStateManager::Get().ChangeResourceStateAndGetBarrier(frameBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET),		
	};
	mCommandList->ResourceBarrier(_countof(barriers), barriers);

	// Clear render targets
	mCommandList->ClearRenderTargetView(frameBufferCpuDesc, DirectX::Colors::Black, 0U, nullptr);
	CHECK_HR(mCommandList->Close());

	// Execute preliminary task
	ID3D12CommandList* cmdLists[] = { mCommandList };
	mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
}
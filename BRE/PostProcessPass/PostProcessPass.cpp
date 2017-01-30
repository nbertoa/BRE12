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
			CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocators[i]);
		}
		CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAllocators[0], commandList);
		commandList->Close();
	}
}

void PostProcessPass::Init(ID3D12Resource& inputColorBuffer) noexcept {
	ASSERT(IsDataValid() == false);
	
	CreateCommandObjects(mCommandAllocators, mCommandList);
	mColorBuffer = &inputColorBuffer;

	// Initialize recorder's PSO
	PostProcessCmdListRecorder::InitPSO();

	// Initialize recorder
	mRecorder.reset(new PostProcessCmdListRecorder());
	mRecorder->Init(inputColorBuffer);

	ASSERT(IsDataValid());
}

void PostProcessPass::Execute(
	ID3D12Resource& outputColorBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferCpuDescriptor) noexcept 
{
	ASSERT(IsDataValid());
	ASSERT(outputColorBufferCpuDescriptor.ptr != 0UL);

	ExecuteBeginTask(outputColorBuffer, outputColorBufferCpuDescriptor);

	// Wait until all previous tasks command lists are executed
	CommandListExecutor::Get().ResetExecutedCommandListCount();
	mRecorder->RecordAndPushCommandLists(outputColorBufferCpuDescriptor);
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
		mCommandList != nullptr &&
		mRecorder.get() != nullptr &&
		mColorBuffer != nullptr;

	return b;
}

void PostProcessPass::ExecuteBeginTask(
	ID3D12Resource& outputColorBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferCpuDescriptor) noexcept {

	ASSERT(IsDataValid());
	ASSERT(outputColorBufferCpuDescriptor.ptr != 0UL);

	// Used to choose a different command list allocator each call.
	static std::uint32_t cmdAllocIndex{ 0U };

	ID3D12CommandAllocator* cmdAlloc{ mCommandAllocators[cmdAllocIndex] };
	cmdAllocIndex = (cmdAllocIndex + 1U) % _countof(mCommandAllocators);

	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCommandList->Reset(cmdAlloc, nullptr));

	// Set barriers
	CD3DX12_RESOURCE_BARRIER barriers[]{
		ResourceStateManager::ChangeResourceStateAndGetBarrier(*mColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		ResourceStateManager::ChangeResourceStateAndGetBarrier(outputColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET),		
	};
	mCommandList->ResourceBarrier(_countof(barriers), barriers);

	// Clear render targets
	mCommandList->ClearRenderTargetView(outputColorBufferCpuDescriptor, DirectX::Colors::Black, 0U, nullptr);
	CHECK_HR(mCommandList->Close());

	// Execute preliminary task
	CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(*mCommandList);
}
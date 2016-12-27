#include "PostProcessPass.h"

#include <d3d12.h>
#include <DirectXColors.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <CommandManager\CommandManager.h>
#include <DXUtils/d3dx12.h>
#include <ResourceManager\ResourceManager.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils\DebugUtils.h>

namespace {
	void CreateCommandObjects(
		ID3D12CommandAllocator* cmdAllocs[SettingsManager::sQueuedFrameCount],
		ID3D12GraphicsCommandList* &cmdList) noexcept {

		ASSERT(SettingsManager::sQueuedFrameCount > 0U);
		ASSERT(cmdList == nullptr);

		// Create command allocators and command list
		for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
			ASSERT(cmdAllocs[i] == nullptr);
			CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocs[i]);
		}
		CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAllocs[0], cmdList);
		cmdList->Close();
	}
}

void PostProcessPass::Init(
	CommandListExecutor& cmdListExecutor,
	ID3D12CommandQueue& cmdQueue,
	ID3D12Resource& colorBuffer) noexcept {

	ASSERT(ValidateData() == false);
	
	CreateCommandObjects(mCmdAllocs, mCmdList);
	mCmdListExecutor = &cmdListExecutor;
	mCmdQueue = &cmdQueue;
	mColorBuffer = &colorBuffer;

	// Initialize recorder's PSO
	PostProcessCmdListRecorder::InitPSO();

	// Initialize recorder
	mRecorder.reset(new PostProcessCmdListRecorder(cmdListExecutor.CmdListQueue()));
	mRecorder->Init(colorBuffer);

	ASSERT(ValidateData());
}

void PostProcessPass::Execute(
	ID3D12Resource& frameBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& frameBufferCpuDesc) noexcept {

	ASSERT(ValidateData());
	ASSERT(frameBufferCpuDesc.ptr != 0UL);

	ExecuteBeginTask(frameBuffer, frameBufferCpuDesc);

	mCmdListExecutor->ResetExecutedCmdListCount();
	mRecorder->RecordAndPushCommandLists(frameBufferCpuDesc);
	
	// Wait until all previous tasks command lists are executed
	while (mCmdListExecutor->ExecutedCmdListCount() < 1) {
		Sleep(0U);
	}
}

bool PostProcessPass::ValidateData() const noexcept {
	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCmdAllocs[i] == nullptr) {
			return false;
		}
	}

	const bool b =
		mCmdListExecutor != nullptr &&
		mCmdQueue != nullptr &&
		mCmdList != nullptr &&
		mRecorder.get() != nullptr &&
		mColorBuffer != nullptr;

	return b;
}

void PostProcessPass::ExecuteBeginTask(
	ID3D12Resource& frameBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& frameBufferCpuDesc) noexcept {

	ASSERT(ValidateData());
	ASSERT(frameBufferCpuDesc.ptr != 0UL);

	// Used to choose a different command list allocator each call.
	static std::uint32_t cmdAllocIndex{ 0U };

	ID3D12CommandAllocator* cmdAlloc{ mCmdAllocs[cmdAllocIndex] };
	cmdAllocIndex = (cmdAllocIndex + 1U) % _countof(mCmdAllocs);

	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCmdList->Reset(cmdAlloc, nullptr));

	// Set barriers
	CD3DX12_RESOURCE_BARRIER barriers[]{
		ResourceStateManager::Get().TransitionState(*mColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		ResourceStateManager::Get().TransitionState(frameBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET),		
	};
	mCmdList->ResourceBarrier(_countof(barriers), barriers);

	// Clear render targets
	mCmdList->ClearRenderTargetView(frameBufferCpuDesc, DirectX::Colors::Black, 0U, nullptr);
	CHECK_HR(mCmdList->Close());

	// Execute preliminary task
	mCmdListExecutor->ResetExecutedCmdListCount();
	ID3D12CommandList* cmdLists[] = { mCmdList };
	mCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
}
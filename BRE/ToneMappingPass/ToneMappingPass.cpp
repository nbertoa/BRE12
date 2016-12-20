#include "ToneMappingPass.h"

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
		ID3D12CommandAllocator* cmdAllocs[Settings::sQueuedFrameCount],
		ID3D12GraphicsCommandList* &cmdList) noexcept {

		ASSERT(Settings::sQueuedFrameCount > 0U);
		ASSERT(cmdList == nullptr);

		// Create command allocators and command list
		for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
			ASSERT(cmdAllocs[i] == nullptr);
			CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocs[i]);
		}
		CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAllocs[0], cmdList);
		cmdList->Close();
	}
}

void ToneMappingPass::Init(
	ID3D12Device& device,
	CommandListExecutor& cmdListExecutor,
	ID3D12CommandQueue& cmdQueue,
	ID3D12Resource& inputColorBuffer,
	ID3D12Resource& outputColorBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& outputBufferCpuDesc,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept {

	ASSERT(ValidateData() == false);
	
	CreateCommandObjects(mCmdAllocs, mCmdList);
	mCmdListExecutor = &cmdListExecutor;
	mCmdQueue = &cmdQueue;
	mInputColorBuffer = &inputColorBuffer;
	mOutputColorBuffer = &outputColorBuffer;

	// Initialize recorder's PSO
	ToneMappingCmdListRecorder::InitPSO();

	// Initialize recorder
	mRecorder.reset(new ToneMappingCmdListRecorder(device, cmdListExecutor.CmdListQueue()));
	mRecorder->Init(*mInputColorBuffer, outputBufferCpuDesc, depthBufferCpuDesc);

	ASSERT(ValidateData());
}

void ToneMappingPass::Execute() noexcept {
	ASSERT(ValidateData());

	ExecuteBeginTask();

	mCmdListExecutor->ResetExecutedCmdListCount();
	mRecorder->RecordAndPushCommandLists();
	
	// Wait until all previous tasks command lists are executed
	while (mCmdListExecutor->ExecutedCmdListCount() < 1) {
		Sleep(0U);
	}
}

bool ToneMappingPass::ValidateData() const noexcept {
	for (std::uint32_t i = 0U; i < Settings::sQueuedFrameCount; ++i) {
		if (mCmdAllocs[i] == nullptr) {
			return false;
		}
	}

	const bool b =
		mCmdListExecutor != nullptr &&
		mCmdQueue != nullptr &&
		mCmdList != nullptr &&
		mRecorder.get() != nullptr &&
		mInputColorBuffer != nullptr &&
		mOutputColorBuffer != nullptr;

	return b;
}

void ToneMappingPass::ExecuteBeginTask() noexcept {

	ASSERT(ValidateData());

	// Used to choose a different command list allocator each call.
	static std::uint32_t cmdAllocIndex{ 0U };

	ID3D12CommandAllocator* cmdAlloc{ mCmdAllocs[cmdAllocIndex] };
	cmdAllocIndex = (cmdAllocIndex + 1U) % _countof(mCmdAllocs);

	CHECK_HR(cmdAlloc->Reset());
	CHECK_HR(mCmdList->Reset(cmdAlloc, nullptr));

	// Set barriers
	CD3DX12_RESOURCE_BARRIER barriers[]{
		ResourceStateManager::Get().TransitionState(*mInputColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		ResourceStateManager::Get().TransitionState(*mOutputColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET),
	};
	mCmdList->ResourceBarrier(_countof(barriers), barriers);

	// Clear render targets
	CHECK_HR(mCmdList->Close());

	// Execute preliminary task
	mCmdListExecutor->ResetExecutedCmdListCount();
	ID3D12CommandList* cmdLists[] = { mCmdList };
	mCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
}
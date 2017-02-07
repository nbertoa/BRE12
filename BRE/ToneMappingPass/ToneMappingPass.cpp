#include "ToneMappingPass.h"

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
		ID3D12CommandAllocator* commandAllocators[SettingsManager::sQueuedFrameCount],
		ID3D12GraphicsCommandList* &commandList) noexcept {

		ASSERT(SettingsManager::sQueuedFrameCount > 0U);
		ASSERT(commandList == nullptr);

		// Create command allocators and command list
		for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
			ASSERT(commandAllocators[i] == nullptr);
			commandAllocators[i] = &CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
		}
		commandList = &CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *commandAllocators[0]);
		commandList->Close();
	}
}

void ToneMappingPass::Init(
	ID3D12Resource& inputColorBuffer,
	ID3D12Resource& outputColorBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& outputBufferCpuDesc) noexcept 
{
	ASSERT(IsDataValid() == false);
	
	CreateCommandObjects(mCommandAllocators, mCommandList);
	mInputColorBuffer = &inputColorBuffer;
	mOutputColorBuffer = &outputColorBuffer;

	ToneMappingCmdListRecorder::InitPSO();

	mCommandListRecorder.reset(new ToneMappingCmdListRecorder());
	mCommandListRecorder->Init(*mInputColorBuffer, outputBufferCpuDesc);

	ASSERT(IsDataValid());
}

void ToneMappingPass::Execute() noexcept {
	ASSERT(IsDataValid());

	ExecuteBeginTask();

	CommandListExecutor::Get().ResetExecutedCommandListCount();
	mCommandListRecorder->RecordAndPushCommandLists();
	while (CommandListExecutor::Get().GetExecutedCommandListCount() < 1) {
		Sleep(0U);
	}
}

bool ToneMappingPass::IsDataValid() const noexcept {
	for (std::uint32_t i = 0U; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCommandAllocators[i] == nullptr) {
			return false;
		}
	}

	const bool b =
		mCommandList != nullptr &&
		mCommandListRecorder.get() != nullptr &&
		mInputColorBuffer != nullptr &&
		mOutputColorBuffer != nullptr;

	return b;
}

void ToneMappingPass::ExecuteBeginTask() noexcept {
	ASSERT(IsDataValid());

	// Check resource states:
	// - Input color buffer was used as render target in previous pass
	// - Output color buffer was used as pixel shader resource in previous pass
	ASSERT(ResourceStateManager::GetResourceState(*mInputColorBuffer) == D3D12_RESOURCE_STATE_RENDER_TARGET);
	ASSERT(ResourceStateManager::GetResourceState(*mOutputColorBuffer) == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// Used to choose a different command list allocator each call.
	static std::uint32_t commandAllocatorIndex{ 0U };

	ID3D12CommandAllocator* commandAllocator{ mCommandAllocators[commandAllocatorIndex] };

	CHECK_HR(commandAllocator->Reset());
	CHECK_HR(mCommandList->Reset(commandAllocator, nullptr));

	// Set barriers
	CD3DX12_RESOURCE_BARRIER barriers[]
	{
		ResourceStateManager::ChangeResourceStateAndGetBarrier(*mInputColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		ResourceStateManager::ChangeResourceStateAndGetBarrier(*mOutputColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET),
	};
	mCommandList->ResourceBarrier(_countof(barriers), barriers);

	CHECK_HR(mCommandList->Close());
	CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(*mCommandList);

	commandAllocatorIndex = (commandAllocatorIndex + 1U) % _countof(mCommandAllocators);
}
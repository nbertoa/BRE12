#include "PostProcessPass.h"

#include <d3d12.h>
#include <DirectXColors.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <DXUtils/d3dx12.h>
#include <ResourceManager\ResourceManager.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils\DebugUtils.h>

void PostProcessPass::Init(ID3D12Resource& inputColorBuffer) noexcept {
	ASSERT(IsDataValid() == false);
	
	mColorBuffer = &inputColorBuffer;

	// Initialize recorder's PSO
	PostProcessCmdListRecorder::InitSharedPSOAndRootSignature();

	// Initialize recorder
	mCommandListRecorder.reset(new PostProcessCmdListRecorder());
	mCommandListRecorder->Init(inputColorBuffer);

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
	mCommandListRecorder->RecordAndPushCommandLists(outputColorBufferCpuDescriptor);
	while (CommandListExecutor::Get().GetExecutedCommandListCount() < 1) {
		Sleep(0U);
	}
}

bool PostProcessPass::IsDataValid() const noexcept {
	const bool b =
		mCommandListRecorder.get() != nullptr &&
		mColorBuffer != nullptr;

	return b;
}

void PostProcessPass::ExecuteBeginTask(
	ID3D12Resource& outputColorBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& outputColorBufferCpuDescriptor) noexcept {

	ASSERT(IsDataValid());
	ASSERT(outputColorBufferCpuDescriptor.ptr != 0UL);

	// Check resource states:
	// - Input color buffer was used as render target in previous pass
	// - Output color buffer is the frame buffer, so it was used to present before.
	ASSERT(ResourceStateManager::GetResourceState(*mColorBuffer) == D3D12_RESOURCE_STATE_RENDER_TARGET);
	ASSERT(ResourceStateManager::GetResourceState(outputColorBuffer) == D3D12_RESOURCE_STATE_PRESENT);

	ID3D12GraphicsCommandList& commandList = mCommandListPerFrame.ResetWithNextCommandAllocator(nullptr);

	// Set barriers
	CD3DX12_RESOURCE_BARRIER barriers[]
	{
		ResourceStateManager::ChangeResourceStateAndGetBarrier(*mColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		ResourceStateManager::ChangeResourceStateAndGetBarrier(outputColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET),		
	};
	commandList.ResourceBarrier(_countof(barriers), barriers);

	commandList.ClearRenderTargetView(outputColorBufferCpuDescriptor, DirectX::Colors::Black, 0U, nullptr);

	CHECK_HR(commandList.Close());
	CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(commandList);
}
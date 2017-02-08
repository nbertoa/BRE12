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
	
	mInputColorBuffer = &inputColorBuffer;

	PostProcessCmdListRecorder::InitSharedPSOAndRootSignature();

	mCommandListRecorder.reset(new PostProcessCmdListRecorder());
	mCommandListRecorder->Init(inputColorBuffer);

	ASSERT(IsDataValid());
}

void PostProcessPass::Execute(
	ID3D12Resource& renderTargetBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept 
{
	ASSERT(IsDataValid());
	ASSERT(renderTargetView.ptr != 0UL);

	ExecuteBeginTask(renderTargetBuffer, renderTargetView);
		
	CommandListExecutor::Get().ResetExecutedCommandListCount();
	mCommandListRecorder->RecordAndPushCommandLists(renderTargetView);

	// Wait until all previous tasks command lists are executed
	while (CommandListExecutor::Get().GetExecutedCommandListCount() < 1) {
		Sleep(0U);
	}
}

bool PostProcessPass::IsDataValid() const noexcept {
	const bool b =
		mCommandListRecorder.get() != nullptr &&
		mInputColorBuffer != nullptr;

	return b;
}

void PostProcessPass::ExecuteBeginTask(
	ID3D12Resource& renderTargetBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept {

	ASSERT(IsDataValid());
	ASSERT(renderTargetView.ptr != 0UL);

	// Check resource states:
	// - Input color buffer was used as render target in previous pass
	// - Output color buffer is the frame buffer, so it was used to present before.
	ASSERT(ResourceStateManager::GetResourceState(*mInputColorBuffer) == D3D12_RESOURCE_STATE_RENDER_TARGET);
	ASSERT(ResourceStateManager::GetResourceState(renderTargetBuffer) == D3D12_RESOURCE_STATE_PRESENT);

	ID3D12GraphicsCommandList& commandList = mCommandListPerFrame.ResetWithNextCommandAllocator(nullptr);

	CD3DX12_RESOURCE_BARRIER barriers[]
	{
		ResourceStateManager::ChangeResourceStateAndGetBarrier(*mInputColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		ResourceStateManager::ChangeResourceStateAndGetBarrier(renderTargetBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET),		
	};
	commandList.ResourceBarrier(_countof(barriers), barriers);

	commandList.ClearRenderTargetView(renderTargetView, DirectX::Colors::Black, 0U, nullptr);

	CHECK_HR(commandList.Close());
	CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(commandList);
}
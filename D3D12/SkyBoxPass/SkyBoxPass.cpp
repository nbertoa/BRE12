#include "SkyBoxPass.h"

#include <cstdint>
#include <d3d12.h>

#include <CommandListProcessor/CommandListProcessor.h>
#include <DXUtils\CBuffers.h>
#include <DXUtils/d3dx12.h>
#include <SkyBoxPass\SkyBoxCmdListRecorder.h>
#include <Utils\DebugUtils.h>

void SkyBoxPass::Init(
	const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept {

	ASSERT(ValidateData() == false);

	ASSERT(mRecorder.get() != nullptr);

	mColorBufferCpuDesc = colorBufferCpuDesc;
	mDepthBufferCpuDesc = depthBufferCpuDesc;

	// Initialize recoders's pso
	SkyBoxCmdListRecorder::InitPSO();

	ASSERT(ValidateData());
}

void SkyBoxPass::Execute(CommandListProcessor& cmdListProcessor, const FrameCBuffer& frameCBuffer) noexcept {
	ASSERT(ValidateData());

	cmdListProcessor.ResetExecutedTasksCounter();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = { mColorBufferCpuDesc };
	mRecorder->RecordCommandLists(frameCBuffer, rtvHandles, _countof(rtvHandles), mDepthBufferCpuDesc);

	// Wait until all previous tasks command lists are executed
	while (cmdListProcessor.ExecutedTasksCounter() < 1U) {
		Sleep(0U);
	}
}

bool SkyBoxPass::ValidateData() const noexcept {
	const bool b =
		mRecorder.get() != nullptr &&
		mColorBufferCpuDesc.ptr != 0UL &&
		mDepthBufferCpuDesc.ptr != 0UL;

	return b;
}
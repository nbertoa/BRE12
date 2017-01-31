#include "LightingPassCmdListRecorder.h"

#include <CommandManager/CommandAllocatorManager.h>
#include <CommandManager/CommandListManager.h>
#include <Utils/DebugUtils.h>

namespace {
	void BuildCommandObjects(
		ID3D12GraphicsCommandList* &commandList, 
		ID3D12CommandAllocator* commandAllocators[], 
		const std::size_t commandAllocatorCount) noexcept 
	{
		ASSERT(commandList == nullptr);

#ifdef _DEBUG
		for (std::uint32_t i = 0U; i < commandAllocatorCount; ++i) {
			ASSERT(commandAllocators[i] == nullptr);
		}
#endif

		for (std::uint32_t i = 0U; i < commandAllocatorCount; ++i) {
			commandAllocators[i] = &CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
		}

		commandList = &CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *commandAllocators[0]);

		commandList->Close();
	}
}

LightingPassCmdListRecorder::LightingPassCmdListRecorder() {
	BuildCommandObjects(mCommandList, mCommandAllocators, _countof(mCommandAllocators));
}

bool LightingPassCmdListRecorder::IsDataValid() const noexcept {
	for (std::uint32_t i = 0UL; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCommandAllocators[i] == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0UL; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mFrameCBuffer[i] == nullptr) {
			return false;
		}
	}

	return
		mCommandList != nullptr &&
		mNumLights != 0UL &&
		mImmutableCBuffer != nullptr &&
		mLightsBuffer != nullptr &&
		mLightsBufferGpuDescBegin.ptr != 0UL;
}

void LightingPassCmdListRecorder::SetOutputColorBufferCpuDescriptor(
	const D3D12_CPU_DESCRIPTOR_HANDLE outputColorBufferCpuDesc) noexcept
{
	ASSERT(outputColorBufferCpuDesc.ptr != 0UL);
	mOutputColorBufferCpuDesc = outputColorBufferCpuDesc;
}
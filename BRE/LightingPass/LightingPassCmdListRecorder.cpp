#include "LightingPassCmdListRecorder.h"

#include <CommandManager/CommandManager.h>
#include <Utils/DebugUtils.h>

namespace {
	void BuildCommandObjects(ID3D12GraphicsCommandList* &cmdList, ID3D12CommandAllocator* cmdAlloc[], const std::size_t cmdAllocCount) noexcept {
		ASSERT(cmdList == nullptr);

#ifdef _DEBUG
		for (std::uint32_t i = 0U; i < cmdAllocCount; ++i) {
			ASSERT(cmdAlloc[i] == nullptr);
		}
#endif

		for (std::uint32_t i = 0U; i < cmdAllocCount; ++i) {
			CommandManager::Get().CreateCmdAlloc(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc[i]);
		}

		CommandManager::Get().CreateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAlloc[0], cmdList);

		cmdList->Close();
	}
}

LightingPassCmdListRecorder::LightingPassCmdListRecorder() {
	BuildCommandObjects(mCmdList, mCmdAlloc, _countof(mCmdAlloc));
}

bool LightingPassCmdListRecorder::ValidateData() const noexcept {
	for (std::uint32_t i = 0UL; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCmdAlloc[i] == nullptr) {
			return false;
		}
	}

	for (std::uint32_t i = 0UL; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mFrameCBuffer[i] == nullptr) {
			return false;
		}
	}

	return
		mCmdList != nullptr &&
		mNumLights != 0UL &&
		mImmutableCBuffer != nullptr &&
		mLightsBuffer != nullptr &&
		mLightsBufferGpuDescBegin.ptr != 0UL;
}

void LightingPassCmdListRecorder::InitInternal(
	tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
	const D3D12_CPU_DESCRIPTOR_HANDLE colorBufferCpuDesc) noexcept
{
	ASSERT(colorBufferCpuDesc.ptr != 0UL);

	mCmdListQueue = &cmdListQueue;
	mColorBufferCpuDesc = colorBufferCpuDesc;
}
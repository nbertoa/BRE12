#include "LightingPassCmdListRecorder.h"

#include <Utils/DebugUtils.h>

bool LightingPassCmdListRecorder::IsDataValid() const noexcept {
	for (std::uint32_t i = 0UL; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mFrameCBuffer[i] == nullptr) {
			return false;
		}
	}

	return
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
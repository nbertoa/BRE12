#include "LightingPassCmdListRecorder.h"

#include <Utils/DebugUtils.h>

bool LightingPassCmdListRecorder::IsDataValid() const noexcept {
	return
		mNumLights != 0UL &&
		mImmutableCBuffer != nullptr &&
		mLightsBuffer != nullptr &&
		mLightsBufferGpuDescriptorBegin.ptr != 0UL;
}

void LightingPassCmdListRecorder::SetOutputColorBufferCpuDescriptor(
	const D3D12_CPU_DESCRIPTOR_HANDLE outputColorBufferCpuDesc) noexcept
{
	ASSERT(outputColorBufferCpuDesc.ptr != 0UL);
	mOutputColorBufferCpuDescriptor = outputColorBufferCpuDesc;
}
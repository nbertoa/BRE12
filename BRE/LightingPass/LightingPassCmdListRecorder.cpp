#include "LightingPassCmdListRecorder.h"

#include <Utils/DebugUtils.h>

bool LightingPassCmdListRecorder::IsDataValid() const noexcept {
	return
		mNumLights != 0UL &&
		mImmutableUploadCBuffer != nullptr &&
		mLightsUploadBuffer != nullptr &&
		mStartLightsBufferShaderResourceView.ptr != 0UL;
}

void LightingPassCmdListRecorder::SetRenderTargetView(
	const D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView) noexcept
{
	ASSERT(renderTargetView.ptr != 0UL);
	mRenderTargetView = renderTargetView;
}
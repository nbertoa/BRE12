#include "GeometryPassCmdListRecorder.h"

#include <ShaderUtils\CBuffers.h>
#include <Utils/DebugUtils.h>

bool 
GeometryPassCmdListRecorder::IsDataValid() const noexcept {
	const std::size_t geometryDataCount{ mGeometryDataVec.size() };
	for (std::size_t i = 0UL; i < geometryDataCount; ++i) {
		const std::size_t numMatrices{ mGeometryDataVec[i].mWorldMatrices.size() };
		if (numMatrices == 0UL) {
			return false;
		}
	}

	return
		mObjectUploadCBuffers != nullptr &&
		mStartObjectCBufferView.ptr != 0UL &&
		geometryDataCount != 0UL &&
		mMaterialUploadCBuffers != nullptr &&
		mStartMaterialCBufferView.ptr != 0UL;
}

void 
GeometryPassCmdListRecorder::Init(
	const D3D12_CPU_DESCRIPTOR_HANDLE* geometryBufferRenderTargetViews,
	const std::uint32_t geometryBufferRenderTargetViewCount,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferView) noexcept
{
	ASSERT(geometryBufferRenderTargetViews != nullptr);
	ASSERT(geometryBufferRenderTargetViewCount != 0U);
	ASSERT(depthBufferView.ptr != 0UL);

	mGeometryBufferRenderTargetViews = geometryBufferRenderTargetViews;
	mGeometryBufferRenderTargetViewCount = geometryBufferRenderTargetViewCount;
	mDepthBufferView = depthBufferView;
}

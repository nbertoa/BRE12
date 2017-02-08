#include "GeometryPassCmdListRecorder.h"

#include <ShaderUtils\CBuffers.h>
#include <Utils/DebugUtils.h>

bool GeometryPassCmdListRecorder::IsDataValid() const noexcept {
	const std::size_t geometryDataCount{ mGeometryDataVec.size() };
	for (std::size_t i = 0UL; i < geometryDataCount; ++i) {
		const std::size_t numMatrices{ mGeometryDataVec[i].mWorldMatrices.size() };
		if (numMatrices == 0UL) {
			return false;
		}
	}

	return
		mObjectCBuffer != nullptr &&
		mObjectCBufferGpuDescBegin.ptr != 0UL &&
		geometryDataCount != 0UL &&
		mMaterialsCBuffer != nullptr &&
		mMaterialsCBufferGpuDescBegin.ptr != 0UL;
}

void GeometryPassCmdListRecorder::Init(
	const D3D12_CPU_DESCRIPTOR_HANDLE* geometryBuffersCpuDescs,
	const std::uint32_t geometryBuffersCpuDescCount,
	const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept
{
	ASSERT(geometryBuffersCpuDescs != nullptr);
	ASSERT(geometryBuffersCpuDescCount != 0U);
	ASSERT(depthBufferCpuDesc.ptr != 0UL);

	mGeometryBuffersCpuDescs = geometryBuffersCpuDescs;
	mGeometryBuffersCpuDescCount = geometryBuffersCpuDescCount;
	mDepthBufferCpuDesc = depthBufferCpuDesc;
}

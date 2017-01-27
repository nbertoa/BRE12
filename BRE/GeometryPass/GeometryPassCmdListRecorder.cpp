#include "GeometryPassCmdListRecorder.h"

#include <CommandManager/CommandAllocatorManager.h>
#include <CommandManager/CommandListManager.h>
#include <ShaderUtils\CBuffers.h>
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
			CommandAllocatorManager::Get().CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc[i]);
		}

		CommandListManager::Get().CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *cmdAlloc[0], cmdList);

		cmdList->Close();
	}
}

GeometryPassCmdListRecorder::GeometryPassCmdListRecorder() {
	BuildCommandObjects(mCmdList, mCmdAlloc, _countof(mCmdAlloc));
}

bool GeometryPassCmdListRecorder::IsDataValid() const noexcept {
	for (std::uint32_t i = 0UL; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCmdAlloc[i] == nullptr) {
			return false;
		}
	}

	const std::size_t numGeomData{ mGeometryDataVec.size() };
	for (std::size_t i = 0UL; i < numGeomData; ++i) {
		const std::size_t numMatrices{ mGeometryDataVec[i].mWorldMatrices.size() };
		if (numMatrices == 0UL) {
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
		mObjectCBuffer != nullptr &&
		mObjectCBufferGpuDescBegin.ptr != 0UL &&
		numGeomData != 0UL &&
		mMaterialsCBuffer != nullptr &&
		mMaterialsCBufferGpuDescBegin.ptr != 0UL;
}

void GeometryPassCmdListRecorder::InitInternal(
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

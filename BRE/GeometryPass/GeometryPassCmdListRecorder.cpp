#include "GeometryPassCmdListRecorder.h"

#include <CommandManager/CommandAllocatorManager.h>
#include <CommandManager/CommandListManager.h>
#include <ShaderUtils\CBuffers.h>
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
			CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[i]);
		}

		CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *commandAllocators[0], commandList);

		commandList->Close();
	}
}

GeometryPassCmdListRecorder::GeometryPassCmdListRecorder() {
	BuildCommandObjects(mCommandList, mCommandAllocators, _countof(mCommandAllocators));
}

bool GeometryPassCmdListRecorder::IsDataValid() const noexcept {
	for (std::uint32_t i = 0UL; i < SettingsManager::sQueuedFrameCount; ++i) {
		if (mCommandAllocators[i] == nullptr) {
			return false;
		}
	}

	const std::size_t geometryDataCount{ mGeometryDataVec.size() };
	for (std::size_t i = 0UL; i < geometryDataCount; ++i) {
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
		mCommandList != nullptr &&
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
